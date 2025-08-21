#pragma once

int AuthSrvCtrl_ProcessServerReady(AuthSrv *srv, CtrlMsg *msg)
{
    UNREFERENCED_PARAMETER(srv);
    assert(msg->msg_id == CtrlMsgId_ServerReady);
    return 0;
}
