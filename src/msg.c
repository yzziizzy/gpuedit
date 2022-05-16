

#include "msg.h"




// NOTE: returns 0 if the message was NOT handled
// returns the first accepting handler's return value if it was
// data is always freed if a free function is provided
int MessagePipe_Send(MessagePipe* pipe, enum MessageType type, void* data, FreeFn f) {
	int ret = 0;
	
	Message msg = {
		.type = type,
		.data = data,
	};
	
	VEC_EACHP(&pipe->handlers, i, h) {
		if(h->fn) ret = h->fn(h->data, &msg);
		if(ret != 0) break;
	}
	
	// free the data that no handler accepted
	if(f) f(data);

	return ret;
}



void MessagePipe_Listen(MessagePipe* pipe, MessageHandlerFn fn, void* data) {
	VEC_PUSH(&pipe->handlers, ((MessageHandler){.fn = fn, .data = data}));
}


void MessagePipe_Unlisten(MessagePipe* pipe, MessageHandlerFn fn, void* data) {
	VEC_EACHP(&pipe->handlers, i, h) {
		if(h->fn = fn && h->data == data) {
			VEC_RM(&pipe->handlers, i);
			return;
		}
	}
}


