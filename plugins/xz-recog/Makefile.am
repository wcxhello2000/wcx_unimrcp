AM_CPPFLAGS                = $(UNIMRCP_PLUGIN_INCLUDES) \
                            -I$(top_srcdir)/plugins/xz_recog/src/base 

plugin_LTLIBRARIES         = xzrecog.la

include_HEADERS          = src/base/xz_recog.h \
                            src/base/xz_speech.h \
                            src/base/xz_tool.h \
                            src/base/xz_link.h \
                            src/base/xz_http.h \
                            src/base/xz_base64.h \
                            src/base/cJSON.h

xzrecog_la_SOURCES       = src/xz_recog_engine.c \
                            src/base/xz_recog.c \
                            src/base/xz_speech.c \
                            src/base/xz_tool.c \
                            src/base/xz_link.c \
                            src/base/xz_http.c \
                            src/base/xz_base64.c \
                            src/base/cJSON.c

xzrecog_la_LDFLAGS       = $(UNIMRCP_PLUGIN_OPTS) -lcurl -lm -lcrypto

include $(top_srcdir)/build/rules/uniplugin.am

