#ifndef __gpuedit_msg_h__
#define __gpuedit_msg_h__



#include "sti/sti.h"


#define MESSAGE_TYPES \
	X(OpenFile) \
	X(OpenFileOpt) \
	X(CloseMe) \
	X(GrepOpener) \
	X(CmdFwd) \
	X(BufferRefDec) \




enum MessageType {
	MSG_NULL = 0,
#define X(a) MSG_##a,
	MESSAGE_TYPES
#undef X
	MSG_MAX_VALUE,
};


typedef void (*FreeFn)(void*);


typedef struct Message {
	enum MessageType type;
	void* data;
} Message;


typedef int (*MessageHandlerFn)(void*, Message*);


typedef struct MessageHandler {
	MessageHandlerFn fn;
	void* data;
} MessageHandler;


typedef struct MessagePipe {
	VEC(MessageHandler) handlers;	
} MessagePipe;



typedef struct MessageFileOpt {
	char* path;
	intptr_t line_num;
	int set_focus;
	int scroll_existing;
} MessageFileOpt;


int MessagePipe_Send(MessagePipe* pipe, enum MessageType type, void* data, FreeFn f);
void MessagePipe_Listen(MessagePipe* pipe, MessageHandlerFn fn, void* data);
void MessagePipe_UnListen(MessagePipe* pipe, MessageHandlerFn fn, void* data);








#endif // __gpuedit_msg_h__
