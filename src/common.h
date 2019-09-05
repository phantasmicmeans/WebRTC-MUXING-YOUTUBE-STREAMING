/*
 * common.h
 *
 *  Created on: 2019. 5. 30.
 *      Author: alnova2
 */

#ifndef COMMON_H_
#define COMMON_H_

typedef struct _sigrtcmsg
{
    char* order;
    char* sdp;
    char* sdptype;
    char* msgextra;
    char* candidate;
    char* sdpMid;
    int sdpMLineIndex;
} sigrtcmsg;

#endif /* COMMON_H_ */
