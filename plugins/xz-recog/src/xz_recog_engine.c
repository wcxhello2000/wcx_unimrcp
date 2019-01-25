/*
 * Copyright 2008-2015 Arsen Chaloyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * Mandatory rules concerning plugin implementation.
 * 1. Each plugin MUST implement a plugin/engine creator function
 *    with the exact signature and name (the main entry point)
 *        MRCP_PLUGIN_DECLARE(mrcp_engine_t*) mrcp_plugin_create(apr_pool_t *pool)
 * 2. Each plugin MUST declare its version number
 *        MRCP_PLUGIN_VERSION_DECLARE
 * 3. One and only one response MUST be sent back to the received request.
 * 4. Methods (callbacks) of the MRCP engine channel MUST not block.
 *   (asynchronous response can be sent from the context of other thread)
 * 5. Methods (callbacks) of the MPF engine stream MUST not block.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mrcp_recog_engine.h"
#include "mpf_activity_detector.h"
#include "apt_consumer_task.h"
#include "apt_log.h"
#include "base/xz_recog.h"

#define RECOG_ENGINE_TASK_NAME "Xz Recog Engine"

int N = 640; //帧长度
int LOW_THRESHOLD = 50; //低阈值

// extern void xz_xf_recog_wav(const char* filename);

typedef struct xz_recog_engine_t xz_recog_engine_t;
typedef struct xz_recog_channel_t xz_recog_channel_t;
typedef struct xz_recog_msg_t xz_recog_msg_t;

/** Declaration of recognizer engine methods */
static apt_bool_t xz_recog_engine_destroy(mrcp_engine_t *engine);
static apt_bool_t xz_recog_engine_open(mrcp_engine_t *engine);
static apt_bool_t xz_recog_engine_close(mrcp_engine_t *engine);
static mrcp_engine_channel_t* xz_recog_engine_channel_create(mrcp_engine_t *engine, apr_pool_t *pool);

static const struct mrcp_engine_method_vtable_t engine_vtable = {
	xz_recog_engine_destroy,
	xz_recog_engine_open,
	xz_recog_engine_close,
	xz_recog_engine_channel_create
};


/** Declaration of recognizer channel methods */
static apt_bool_t xz_recog_channel_destroy(mrcp_engine_channel_t *channel);
static apt_bool_t xz_recog_channel_open(mrcp_engine_channel_t *channel);
static apt_bool_t xz_recog_channel_close(mrcp_engine_channel_t *channel);
static apt_bool_t xz_recog_channel_request_process(mrcp_engine_channel_t *channel, mrcp_message_t *request);

static const struct mrcp_engine_channel_method_vtable_t channel_vtable = {
	xz_recog_channel_destroy,
	xz_recog_channel_open,
	xz_recog_channel_close,
	xz_recog_channel_request_process
};

/** Declaration of recognizer audio stream methods */
static apt_bool_t xz_recog_stream_destroy(mpf_audio_stream_t *stream);
static apt_bool_t xz_recog_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec);
static apt_bool_t xz_recog_stream_close(mpf_audio_stream_t *stream);
static apt_bool_t xz_recog_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame);

static apt_bool_t xz_recog_stream_buffer(xz_recog_channel_t *recog_channel, const void *voice_data, unsigned int voice_len);
static apt_bool_t xz_recog_start_recog(xz_recog_channel_t *recog_channel);
static apt_bool_t xz_recog_pcm_stranslation_wav(const char *source, const char *dest);
//帧幅度，采样值绝对值和
static apt_bool_t xz_recog_amplitudes(const char *source_filename, const char *dest_filename);
static apt_bool_t xz_recog_add_prev_data(xz_recog_channel_t *recog_channel);


static const mpf_audio_stream_vtable_t audio_stream_vtable = {
	xz_recog_stream_destroy,
	NULL,
	NULL,
	NULL,
	xz_recog_stream_open,
	xz_recog_stream_close,
	xz_recog_stream_write,
	NULL
};

/** Declaration of xz recognizer engine */
struct xz_recog_engine_t {
	apt_consumer_task_t    *task;
};

/** Declaration of xz recognizer channel */
struct xz_recog_channel_t {
	/** Back pointer to engine */
	xz_recog_engine_t     *xz_engine;
	/** Engine channel base */
	mrcp_engine_channel_t   *channel;

	/** Active (in-progress) recognition request */
	mrcp_message_t          *recog_request;
	/** Pending stop response */
	mrcp_message_t          *stop_response;
	/** Indicates whether input timers are started */
	apt_bool_t               timers_started;
	/** Voice activity detector */
	mpf_activity_detector_t *detector;
	/** File to write utterance to */
	FILE                    *audio_out;
	/** File to write buffer data */
	FILE					*buffer_out;
	char					*buffer_filename;
	char 					*buffer_wav_filename;
	char					*temp_filename;
	char  					*last_result;
	int 					rate;
	mpf_detector_event_e    xz_detector_state;
	LINK_STACK				*prev_data_link_stack;
};

typedef enum {
	DEMO_RECOG_MSG_OPEN_CHANNEL,
	DEMO_RECOG_MSG_CLOSE_CHANNEL,
	DEMO_RECOG_MSG_REQUEST_PROCESS
} xz_recog_msg_type_e;

/** Declaration of xz recognizer task message */
struct xz_recog_msg_t {
	xz_recog_msg_type_e  type;
	mrcp_engine_channel_t *channel; 
	mrcp_message_t        *request;
};

static apt_bool_t xz_recog_msg_signal(xz_recog_msg_type_e type, mrcp_engine_channel_t *channel, mrcp_message_t *request);
static apt_bool_t xz_recog_msg_process(apt_task_t *task, apt_task_msg_t *msg);

/** Declare this macro to set plugin version */
MRCP_PLUGIN_VERSION_DECLARE

/**
 * Declare this macro to use log routine of the server, plugin is loaded from.
 * Enable/add the corresponding entry in logger.xml to set a cutsom log source priority.
 *    <source name="RECOG-PLUGIN" priority="DEBUG" masking="NONE"/>
 */
MRCP_PLUGIN_LOG_SOURCE_IMPLEMENT(RECOG_PLUGIN,"RECOG-PLUGIN")

/** Use custom log source mark */
#define RECOG_LOG_MARK   APT_LOG_MARK_DECLARE(RECOG_PLUGIN)

/** Create xz recognizer engine */
MRCP_PLUGIN_DECLARE(mrcp_engine_t*) mrcp_plugin_create(apr_pool_t *pool)
{
	xz_recog_engine_t *xz_engine = apr_palloc(pool,sizeof(xz_recog_engine_t));
	apt_task_t *task;
	apt_task_vtable_t *vtable;
	apt_task_msg_pool_t *msg_pool;

	msg_pool = apt_task_msg_pool_create_dynamic(sizeof(xz_recog_msg_t),pool);
	xz_engine->task = apt_consumer_task_create(xz_engine,msg_pool,pool);
	if(!xz_engine->task) {
		return NULL;
	}
	task = apt_consumer_task_base_get(xz_engine->task);
	apt_task_name_set(task,RECOG_ENGINE_TASK_NAME);
	vtable = apt_task_vtable_get(task);
	if(vtable) {
		vtable->process_msg = xz_recog_msg_process;
	}

	/* create engine base */
	return mrcp_engine_create(
				MRCP_RECOGNIZER_RESOURCE,  /* MRCP resource identifier */
				xz_engine,               /* object to associate */
				&engine_vtable,            /* virtual methods table of engine */
				pool);                     /* pool to allocate memory from */
}

/** Destroy recognizer engine */
static apt_bool_t xz_recog_engine_destroy(mrcp_engine_t *engine)
{
	xz_recog_engine_t *xz_engine = engine->obj;
	if(xz_engine->task) {
		apt_task_t *task = apt_consumer_task_base_get(xz_engine->task);
		apt_task_destroy(task);
		xz_engine->task = NULL;
	}
	return TRUE;
}

/** Open recognizer engine */
static apt_bool_t xz_recog_engine_open(mrcp_engine_t *engine)
{
	xz_recog_engine_t *xz_engine = engine->obj;
	if(xz_engine->task) {
		apt_task_t *task = apt_consumer_task_base_get(xz_engine->task);
		apt_task_start(task);
	}
	return mrcp_engine_open_respond(engine,TRUE);
}

/** Close recognizer engine */
static apt_bool_t xz_recog_engine_close(mrcp_engine_t *engine)
{
	xz_recog_engine_t *xz_engine = engine->obj;
	if(xz_engine->task) {
		apt_task_t *task = apt_consumer_task_base_get(xz_engine->task);
		apt_task_terminate(task,TRUE);
	}
	return mrcp_engine_close_respond(engine);
}

static mrcp_engine_channel_t* xz_recog_engine_channel_create(mrcp_engine_t *engine, apr_pool_t *pool)
{
	mpf_stream_capabilities_t *capabilities;
	mpf_termination_t *termination; 
    
	/* create xz recog channel */
	xz_recog_channel_t *recog_channel = apr_palloc(pool,sizeof(xz_recog_channel_t));
	recog_channel->xz_engine = engine->obj;
	recog_channel->recog_request = NULL;
	recog_channel->stop_response = NULL;
	recog_channel->detector = mpf_activity_detector_create(pool);
	recog_channel->audio_out = NULL;
	recog_channel->buffer_out = NULL;
	recog_channel->buffer_filename = NULL;
	recog_channel->buffer_wav_filename = NULL;
	recog_channel->temp_filename = NULL;
	recog_channel->last_result = NULL;
	recog_channel->xz_detector_state = MPF_DETECTOR_EVENT_NONE;
	// apr_size_t xz_size = recog_channel->detector->speech_timeout;
	recog_channel->prev_data_link_stack = xz_creat_link_stack(30);
	
	capabilities = mpf_sink_stream_capabilities_create(pool);
	mpf_codec_capabilities_add(
			&capabilities->codecs,
			MPF_SAMPLE_RATE_8000 | MPF_SAMPLE_RATE_16000,
			"LPCM");

	/* create media termination */
	termination = mrcp_engine_audio_termination_create(
			recog_channel,        /* object to associate */
			&audio_stream_vtable, /* virtual methods table of audio stream */
			capabilities,         /* stream capabilities */
			pool);                /* pool to allocate memory from */

	/* create engine channel base */
	recog_channel->channel = mrcp_engine_channel_create(
			engine,               /* engine */
			&channel_vtable,      /* virtual methods table of engine channel */
			recog_channel,        /* object to associate */
			termination,          /* associated media termination */
			pool);                /* pool to allocate memory from */


	return recog_channel->channel;
}

/** Destroy engine channel */
static apt_bool_t xz_recog_channel_destroy(mrcp_engine_channel_t *channel)
{
	/* nothing to destrtoy */
	xz_recog_channel_t *recog_channel = channel->method_obj;
	xz_destory_link_stack(recog_channel->prev_data_link_stack);
	// remove(recog_channel->buffer_filename);
	return TRUE;
}

/** Open engine channel (asynchronous response MUST be sent)*/
static apt_bool_t xz_recog_channel_open(mrcp_engine_channel_t *channel)
{
	return xz_recog_msg_signal(DEMO_RECOG_MSG_OPEN_CHANNEL,channel,NULL);
}

/** Close engine channel (asynchronous response MUST be sent)*/
static apt_bool_t xz_recog_channel_close(mrcp_engine_channel_t *channel)
{
	return xz_recog_msg_signal(DEMO_RECOG_MSG_CLOSE_CHANNEL,channel,NULL);
}

/** Process MRCP channel request (asynchronous response MUST be sent)*/
static apt_bool_t xz_recog_channel_request_process(mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
	return xz_recog_msg_signal(DEMO_RECOG_MSG_REQUEST_PROCESS,channel,request);
}

/** Process RECOGNIZE request */
static apt_bool_t xz_recog_channel_recognize(mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
	/* process RECOGNIZE request */
	mrcp_recog_header_t *recog_header;
	xz_recog_channel_t *recog_channel = channel->method_obj;
	const mpf_codec_descriptor_t *descriptor = mrcp_engine_sink_stream_codec_get(channel);

	if(!descriptor) {
		apt_log(RECOG_LOG_MARK,APT_PRIO_WARNING,"Failed to Get Codec Descriptor " APT_SIDRES_FMT, MRCP_MESSAGE_SIDRES(request));
		response->start_line.status_code = MRCP_STATUS_CODE_METHOD_FAILED;
		return FALSE;
	}
	
	recog_channel->rate = descriptor->sampling_rate;
	recog_channel->timers_started = TRUE;
	printf("################ sampling_rate=%d\n", recog_channel->rate);

	/* get recognizer header */
	recog_header = mrcp_resource_header_get(request);
	if(recog_header) {
		if(mrcp_resource_header_property_check(request,RECOGNIZER_HEADER_START_INPUT_TIMERS) == TRUE) {
			recog_channel->timers_started = recog_header->start_input_timers;
		}
		if(mrcp_resource_header_property_check(request,RECOGNIZER_HEADER_NO_INPUT_TIMEOUT) == TRUE) {
			mpf_activity_detector_noinput_timeout_set(recog_channel->detector,recog_header->no_input_timeout);
		}
		if(mrcp_resource_header_property_check(request,RECOGNIZER_HEADER_SPEECH_COMPLETE_TIMEOUT) == TRUE) {
			mpf_activity_detector_silence_timeout_set(recog_channel->detector,recog_header->speech_complete_timeout);
		}
	}

	if(!recog_channel->audio_out) {
		const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
		char *file_name = apr_psprintf(channel->pool,"utter-%dkHz-%s.pcm",
							descriptor->sampling_rate/1000,
							request->channel_id.session_id.buf);
		char *file_path = apt_vardir_filepath_get(dir_layout,file_name,channel->pool);
		if(file_path) {
			apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Open Utterance Output File [%s] for Writing",file_path);
			recog_channel->audio_out = fopen(file_path,"wb");
			if(!recog_channel->audio_out) {
				apt_log(RECOG_LOG_MARK,APT_PRIO_WARNING,"Failed to Open Utterance Output File [%s] for Writing",file_path);
			}
		}
	}

	if(!recog_channel->buffer_filename){
		const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
		char *file_name = apr_psprintf(channel->pool,"buffer-%dkHz=%s.pcm",
							descriptor->sampling_rate/1000,
							request->channel_id.session_id.buf);
		recog_channel->buffer_filename = apt_vardir_filepath_get(dir_layout,file_name,channel->pool);
		if(recog_channel->buffer_filename){
			apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Open buffer Output File [%s] for Writing",recog_channel->buffer_filename);
			recog_channel->buffer_out = fopen(recog_channel->buffer_filename,"wb");
			if(!recog_channel->buffer_out){
				apt_log(RECOG_LOG_MARK,APT_PRIO_WARNING,"Failed to Open buffer Output File [%s] for Writing",recog_channel->buffer_filename);
			}
		}
	}
	if(!recog_channel->buffer_wav_filename){
		const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
		char *file_name = apr_psprintf(channel->pool,"buffer-%dkHz=%s.wav",
							descriptor->sampling_rate/1000,
							request->channel_id.session_id.buf);
		recog_channel->buffer_wav_filename = apt_vardir_filepath_get(dir_layout,file_name,channel->pool);
	}
	if(!recog_channel->temp_filename){
		const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
		char *file_name = apr_psprintf(channel->pool,"temp-%dkHz=%s.pcm",
							descriptor->sampling_rate/1000,
							request->channel_id.session_id.buf);
		recog_channel->temp_filename = apt_vardir_filepath_get(dir_layout,file_name,channel->pool);
	}

	response->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
	/* send asynchronous response */
	mrcp_engine_channel_message_send(channel,response);
	recog_channel->recog_request = request;
	return TRUE;
}

/** Process STOP request */
static apt_bool_t xz_recog_channel_stop(mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
	/* process STOP request */
	xz_recog_channel_t *recog_channel = channel->method_obj;
	/* store STOP request, make sure there is no more activity and only then send the response */
	recog_channel->stop_response = response;
	return TRUE;
}

/** Process START-INPUT-TIMERS request */
static apt_bool_t xz_recog_channel_timers_start(mrcp_engine_channel_t *channel, mrcp_message_t *request, mrcp_message_t *response)
{
	xz_recog_channel_t *recog_channel = channel->method_obj;
	recog_channel->timers_started = TRUE;
	return mrcp_engine_channel_message_send(channel,response);
}

/** Dispatch MRCP request */
static apt_bool_t xz_recog_channel_request_dispatch(mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
	apt_bool_t processed = FALSE;
	mrcp_message_t *response = mrcp_response_create(request,request->pool);
	switch(request->start_line.method_id) {
		case RECOGNIZER_SET_PARAMS:
			break;
		case RECOGNIZER_GET_PARAMS:
			break;
		case RECOGNIZER_DEFINE_GRAMMAR:
			break;
		case RECOGNIZER_RECOGNIZE:
			processed = xz_recog_channel_recognize(channel,request,response);
			break;
		case RECOGNIZER_GET_RESULT:
			break;
		case RECOGNIZER_START_INPUT_TIMERS:
			processed = xz_recog_channel_timers_start(channel,request,response);
			break;
		case RECOGNIZER_STOP:
			processed = xz_recog_channel_stop(channel,request,response);
			break;
		default:
			break;
	}
	if(processed == FALSE) {
		/* send asynchronous response for not handled request */
		mrcp_engine_channel_message_send(channel,response);
	}
	return TRUE;
}

/** Callback is called from MPF engine context to destroy any additional data associated with audio stream */
static apt_bool_t xz_recog_stream_destroy(mpf_audio_stream_t *stream)
{
	return TRUE;
}

/** Callback is called from MPF engine context to perform any action before open */
static apt_bool_t xz_recog_stream_open(mpf_audio_stream_t *stream, mpf_codec_t *codec)
{
	return TRUE;
}

/** Callback is called from MPF engine context to perform any action after close */
static apt_bool_t xz_recog_stream_close(mpf_audio_stream_t *stream)
{
	return TRUE;
}

/* Raise xz START-OF-INPUT event */
static apt_bool_t xz_recog_start_of_input(xz_recog_channel_t *recog_channel)
{
	/* create START-OF-INPUT event */
	mrcp_message_t *message = mrcp_event_create(
						recog_channel->recog_request,
						RECOGNIZER_START_OF_INPUT,
						recog_channel->recog_request->pool);
	if(!message) {
		return FALSE;
	}

	/* set request state */
	message->start_line.request_state = MRCP_REQUEST_STATE_INPROGRESS;
	/* send asynch event */
	return mrcp_engine_channel_message_send(recog_channel->channel,message);
}

/* Load xz recognition result */
static apt_bool_t xz_recog_result_load(xz_recog_channel_t *recog_channel, mrcp_message_t *message)
{
	FILE *file;
	mrcp_engine_channel_t *channel = recog_channel->channel;
	const apt_dir_layout_t *dir_layout = channel->engine->dir_layout;
	char *file_path = apt_datadir_filepath_get(dir_layout,"result.xml",message->pool);
	if(!file_path) {
		return FALSE;
	}
	
	/* read the xz result from file */
	file = fopen(file_path,"r");
	if(file) {
		mrcp_generic_header_t *generic_header;
		char text[1024];
		apr_size_t size;
		size = fread(text,1,sizeof(text),file);
		apt_string_assign_n(&message->body,text,size,message->pool);
		fclose(file);

		/* get/allocate generic header */
		generic_header = mrcp_generic_header_prepare(message);
		if(generic_header) {
			/* set content types */
			apt_string_assign(&generic_header->content_type,"application/x-nlsml",message->pool);
			mrcp_generic_header_property_add(message,GENERIC_HEADER_CONTENT_TYPE);
		}
	}
	return TRUE;
}

/* Load xz recognition result */
static apt_bool_t xz_recog_result_load_01(xz_recog_channel_t *recog_channel, mrcp_message_t *message)
{
	apt_str_t *body = &message->body;
	if(!recog_channel->last_result) {
		return FALSE;
	}

	body->buf = apr_psprintf(message->pool,
		"<?xml version=\"1.0\"?>\n"
		"<result>\n"
		"  <interpretation confidence=\"%d\">\n"
		"    <instance>%s</instance>\n"
		"    <input mode=\"speech\">%s</input>\n"
		"  </interpretation>\n"
		"</result>\n",
		99,
		recog_channel->last_result,
		recog_channel->last_result);
	if(body->buf) {
		mrcp_generic_header_t *generic_header;
		generic_header = mrcp_generic_header_prepare(message);
		if(generic_header) {
			/* set content type */
			apt_string_assign(&generic_header->content_type,"application/x-nlsml",message->pool);
			mrcp_generic_header_property_add(message,GENERIC_HEADER_CONTENT_TYPE);
		}
		
		body->length = strlen(body->buf);
	}
	if(recog_channel->last_result) {
		free(recog_channel->last_result);
		recog_channel->last_result = NULL;
	}
	return TRUE;
}

/* Raise xz RECOGNITION-COMPLETE event */
static apt_bool_t xz_recog_recognition_complete(xz_recog_channel_t *recog_channel, mrcp_recog_completion_cause_e cause)
{
	mrcp_recog_header_t *recog_header;
	/* create RECOGNITION-COMPLETE event */
	mrcp_message_t *message = mrcp_event_create(
						recog_channel->recog_request,
						RECOGNIZER_RECOGNITION_COMPLETE,
						recog_channel->recog_request->pool);
	if(!message) {
		return FALSE;
	}
	
	/* get/allocate recognizer header */
	recog_header = mrcp_resource_header_prepare(message);
	if(recog_header) {
		/* set completion cause */
		recog_header->completion_cause = cause;
		mrcp_resource_header_property_add(message,RECOGNIZER_HEADER_COMPLETION_CAUSE);
	}
	/* set request state */
	message->start_line.request_state = MRCP_REQUEST_STATE_COMPLETE;

	if(cause == RECOGNIZER_COMPLETION_CAUSE_SUCCESS) {	
		// xz_recog_result_load(recog_channel,message);
		xz_recog_start_recog(recog_channel);
		xz_recog_result_load_01(recog_channel,message);
	}
	
	recog_channel->recog_request = NULL;
	/* send asynch event */
	return mrcp_engine_channel_message_send(recog_channel->channel,message);
}

static apt_bool_t xz_recog_start_recog(xz_recog_channel_t *recog_channel)
{
	if(recog_channel->buffer_out)
	{
		fclose(recog_channel->buffer_out);
		recog_channel->buffer_out = NULL;
	}

	//此处进行vad特征提取
	// xz_recog_amplitudes(recog_channel->buffer_filename, recog_channel->temp_filename);

	//将pcm格式转换成wav格式
	// xz_recog_pcm_stranslation_wav(recog_channel->temp_filename,recog_channel->buffer_wav_filename);
	xz_recog_pcm_stranslation_wav(recog_channel->buffer_filename,recog_channel->buffer_wav_filename);
	
	//此处调用第三方http接口获取识别结果
	char* rslt = xz_xf_recog_wav(recog_channel->buffer_wav_filename, recog_channel->rate);
	printf("########################xz rslt:[%s]\n", rslt);
	// apt_log(RECOG_LOG_MARK,APT_PRIO_INFO, "rslt:[%s]", rslt);

	//此处将识别结果存放在recog_channel中的last_result中
	if(NULL == recog_channel->last_result) {
		recog_channel->last_result = rslt;
	}
	
	//打开buffer_filename文件，并清空里面的内容
	if(!recog_channel->buffer_out)
	{
		recog_channel->buffer_out = fopen(recog_channel->buffer_filename,"wb");
	}

	// remove(recog_channel->temp_filename);
	remove(recog_channel->buffer_wav_filename);

    return TRUE;
}

static apt_bool_t xz_recog_stream_buffer(xz_recog_channel_t *recog_channel, const void *voice_data, unsigned int voice_len)
{
	if(recog_channel->buffer_out){
		fwrite(voice_data,1,voice_len,recog_channel->buffer_out);
	}

	return TRUE;
}

static apt_bool_t xz_recog_pcm_stranslation_wav(const char *source, const char *dest)
{
	int pcm_len = 0;
	void *pcm_buffer = NULL;
	FILE *pcm_fd = fopen(source, "rb");
	if (pcm_fd == NULL){
		return FALSE;
	}
	fseek(pcm_fd, 0, SEEK_END);
	pcm_len = ftell(pcm_fd);
	fseek(pcm_fd, 0, SEEK_SET);
	pcm_buffer = malloc(pcm_len);
	pcm_len = fread(pcm_buffer, 1, pcm_len, pcm_fd);
	fclose(pcm_fd);

	//44字节wav头
	void *wav_buffer = malloc(pcm_len + 44);
	//wav文件多了44个字节
	int wav_len = pcm_len + 44;
	//添加wav文件头
	memcpy(wav_buffer, "RIFF", 4);
	*(int *)((char*)wav_buffer + 4) = pcm_len + 36;
	memcpy(((char*)wav_buffer + 8), "WAVEfmt ", 8);
	*(int *)((char*)wav_buffer + 16) = 16;
	*(short *)((char*)wav_buffer + 20) = 1;
	*(short *)((char*)wav_buffer + 22) = 1;
	*(int *)((char*)wav_buffer + 24) = 8000;
	*(int *)((char*)wav_buffer + 28) = 16000;
	*(short *)((char*)wav_buffer + 32) = 16 / 8;
	*(short *)((char*)wav_buffer + 34) = 16;
	strcpy((char*)((char*)wav_buffer + 36), "data");
	*(int *)((char*)wav_buffer + 40) = pcm_len;
	//拷贝pcm数据到wav中
	memcpy((char*)wav_buffer + 44, pcm_buffer, pcm_len);

	FILE *wav_fd = fopen(dest, "wb");
	fwrite(wav_buffer, 1, wav_len, wav_fd);
	fclose(wav_fd);

	if(pcm_buffer){
		free(pcm_buffer);
	}
	if(wav_buffer){
		free(wav_buffer);
	}
	
	return TRUE;
}

//帧幅度，绝对值和
static apt_bool_t xz_recog_amplitudes(const char *source_filename, const char *dest_filename)
{
	int sum = 0;
    FILE *rfd = fopen(source_filename, "rb+");
    FILE *wfd = fopen(dest_filename, "wb+");
	unsigned char *sample = (unsigned char*)malloc(2*N);
	
	while(!feof(rfd))
	{	
        memset(sample, 0, 2*N);	
		short *samplenum = NULL;
        int n = fread(sample, 2, N, rfd);
        
        unsigned int i;
        for(i=0; i<n; i++)
        {
            samplenum = (short*)(sample+2*i);
            short tmp = *samplenum;
            if(tmp > 0)
            {
                sum += tmp;
            }
            else
            {
                sum -= tmp;
            }
        }
		if(i > 0)
		{
			sum = sum/i;	
        	if(sum > LOW_THRESHOLD)
        	{
            	fwrite(sample, 2, n, wfd);
        	}
		}
		sum = 0;
	}
   
    if(sample != NULL)
    {
        free(sample);
        sample = NULL;
    }
	fclose(rfd);
    fclose(wfd);
	return TRUE;
}

static apt_bool_t xz_recog_add_prev_data(xz_recog_channel_t *recog_channel)
{
	if(recog_channel->buffer_out){
		// fwrite(voice_data,1,voice_len,recog_channel->buffer_out);
		LINK_STACK	*stack = recog_channel->prev_data_link_stack;
		if(stack->cur_size > 0 && stack->link_head){
			PREV_DATA_NODE_H* head = stack->link_head;
			PREV_DATA_NODE_T* node = head->next;
			while(node){
				fwrite(node->data, 1, node->len, recog_channel->buffer_out);
				node = node->next;
			}
		}
	}
	return TRUE;
}

/** Callback is called from MPF engine context to write/send new frame */
static apt_bool_t xz_recog_stream_write(mpf_audio_stream_t *stream, const mpf_frame_t *frame)
{
	xz_recog_channel_t *recog_channel = stream->obj;
	if(recog_channel->stop_response) {
		/* send asynchronous response to STOP request */
		mrcp_engine_channel_message_send(recog_channel->channel,recog_channel->stop_response);
		recog_channel->stop_response = NULL;
		recog_channel->recog_request = NULL;
		return TRUE;
	}
    
	if(frame->codec_frame.size){
		if(recog_channel->prev_data_link_stack){
			xz_insert_voice_data_into_link_stack(recog_channel->prev_data_link_stack, frame->codec_frame.buffer, frame->codec_frame.size);
		}
	}
	if(recog_channel->recog_request) {

		if(frame->codec_frame.size && recog_channel->xz_detector_state == MPF_DETECTOR_EVENT_ACTIVITY){
			xz_recog_stream_buffer(recog_channel, frame->codec_frame.buffer, frame->codec_frame.size);
    	}
		
		mpf_detector_event_e det_event = mpf_activity_detector_process(recog_channel->detector,frame);
		switch(det_event) {
			case MPF_DETECTOR_EVENT_ACTIVITY:
				if(frame->codec_frame.size){
					xz_recog_add_prev_data(recog_channel);
					xz_recog_stream_buffer(recog_channel, frame->codec_frame.buffer, frame->codec_frame.size);
    			}
				recog_channel->xz_detector_state = MPF_DETECTOR_EVENT_ACTIVITY;

				apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Detected Voice Activity" APT_SIDRES_FMT,
					MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
				xz_recog_start_of_input(recog_channel);
				printf("########################xz Detected Voice Activity\n");
				// apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"########################xz11 Detected Voice Activity");
				break;
			case MPF_DETECTOR_EVENT_INACTIVITY:
				recog_channel->xz_detector_state = MPF_DETECTOR_EVENT_INACTIVITY;
				apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Detected Voice Inactivity " APT_SIDRES_FMT,
					MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
				xz_recog_recognition_complete(recog_channel,RECOGNIZER_COMPLETION_CAUSE_SUCCESS);
				printf("########################xz Detected Voice Inactivity\n");
				// apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"########################xz11 Detected Voice Inactivity");
				break;
			case MPF_DETECTOR_EVENT_NOINPUT:
				recog_channel->xz_detector_state = MPF_DETECTOR_EVENT_NOINPUT;
				apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Detected Noinput " APT_SIDRES_FMT,
					MRCP_MESSAGE_SIDRES(recog_channel->recog_request));
				if(recog_channel->timers_started == TRUE) {
					xz_recog_recognition_complete(recog_channel,RECOGNIZER_COMPLETION_CAUSE_NO_INPUT_TIMEOUT);
				}
				printf("########################xz Detected Noinput\n");
				// apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"########################xz11 Detected Voice Noinput");
				break;
			default:
				break;
		}

		if(recog_channel->recog_request) {
			if((frame->type & MEDIA_FRAME_TYPE_EVENT) == MEDIA_FRAME_TYPE_EVENT) {
				if(frame->marker == MPF_MARKER_START_OF_EVENT) {
					apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Detected Start of Event " APT_SIDRES_FMT " id:%d",
						MRCP_MESSAGE_SIDRES(recog_channel->recog_request),
						frame->event_frame.event_id);
				}
				else if(frame->marker == MPF_MARKER_END_OF_EVENT) {
					apt_log(RECOG_LOG_MARK,APT_PRIO_INFO,"Detected End of Event " APT_SIDRES_FMT " id:%d duration:%d ts",
						MRCP_MESSAGE_SIDRES(recog_channel->recog_request),
						frame->event_frame.event_id,
						frame->event_frame.duration);
				}
			}
		}
		if(recog_channel->audio_out) {
			fwrite(frame->codec_frame.buffer,1,frame->codec_frame.size,recog_channel->audio_out);
		}
	}
	return TRUE;
}

static apt_bool_t xz_recog_msg_signal(xz_recog_msg_type_e type, mrcp_engine_channel_t *channel, mrcp_message_t *request)
{
	apt_bool_t status = FALSE;
	xz_recog_channel_t *xz_channel = channel->method_obj;
	xz_recog_engine_t *xz_engine = xz_channel->xz_engine;
	apt_task_t *task = apt_consumer_task_base_get(xz_engine->task);
	apt_task_msg_t *msg = apt_task_msg_get(task);
	if(msg) {
		xz_recog_msg_t *xz_msg;
		msg->type = TASK_MSG_USER;
		xz_msg = (xz_recog_msg_t*) msg->data;

		xz_msg->type = type;
		xz_msg->channel = channel;
		xz_msg->request = request;
		status = apt_task_msg_signal(task,msg);
	}
	return status;
}

static apt_bool_t xz_recog_msg_process(apt_task_t *task, apt_task_msg_t *msg)
{
	xz_recog_msg_t *xz_msg = (xz_recog_msg_t*)msg->data;
	switch(xz_msg->type) {
		case DEMO_RECOG_MSG_OPEN_CHANNEL:
			/* open channel and send asynch response */
			mrcp_engine_channel_open_respond(xz_msg->channel,TRUE);
			break;
		case DEMO_RECOG_MSG_CLOSE_CHANNEL:
		{
			/* close channel, make sure there is no activity and send asynch response */
			xz_recog_channel_t *recog_channel = xz_msg->channel->method_obj;
			if(recog_channel->audio_out) {
				fclose(recog_channel->audio_out);
				recog_channel->audio_out = NULL;
			}
			if(recog_channel->buffer_out)
			{
				fclose(recog_channel->buffer_out);
				recog_channel->buffer_out = NULL;
			}

			mrcp_engine_channel_close_respond(xz_msg->channel);
			break;
		}
		case DEMO_RECOG_MSG_REQUEST_PROCESS:
			xz_recog_channel_request_dispatch(xz_msg->channel,xz_msg->request);
			break;
		default:
			break;
	}
	return TRUE;
}
