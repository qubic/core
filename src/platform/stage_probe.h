#pragma once

static constexpr unsigned int MAIN_STAGE_LOOP_TOP = 100;
static constexpr unsigned int MAIN_STAGE_CLOCK = 101;
static constexpr unsigned int MAIN_STAGE_PEER_RECEIVE_TRANSMIT = 110;
static constexpr unsigned int MAIN_STAGE_PEER_RECONNECT = 111;
static constexpr unsigned int MAIN_STAGE_PEER_MACHINE_TIMEOUT = 112;
static constexpr unsigned int MAIN_STAGE_PEER_REFRESH = 113;
static constexpr unsigned int MAIN_STAGE_RESPONSE_QUEUE = 120;
static constexpr unsigned int MAIN_STAGE_STATE_SAVE = 130;
static constexpr unsigned int MAIN_STAGE_CLOSE_ALL_PEERS = 140;
static constexpr unsigned int MAIN_STAGE_KEY_PRESSES = 150;
static constexpr unsigned int MAIN_STAGE_NODE_STATE_SAVE = 160;
static constexpr unsigned int MAIN_STAGE_LOGGING = 170;
static constexpr unsigned int MAIN_STAGE_ASYNC_IO_FLUSH = 190;

static constexpr unsigned int TICK_STAGE_RUNNING = 200;
static constexpr unsigned int TICK_STAGE_TRANSITION_WAIT_PROCESSORS = 210;
static constexpr unsigned int TICK_STAGE_END_EPOCH = 211;
static constexpr unsigned int TICK_STAGE_ANT_EXPORT = 212;
static constexpr unsigned int TICK_STAGE_SYSTEM_SAVE = 213;
static constexpr unsigned int TICK_STAGE_BEGIN_EPOCH = 214;
static constexpr unsigned int TICK_STAGE_APPLY_TASK = 215;
static constexpr unsigned int TICK_STAGE_STATE_SAVE = 216;
static constexpr unsigned int TICK_STAGE_TRANSITION_DONE = 217;

static constexpr unsigned int EFI_STAGE_POLL = 300;
static constexpr unsigned int EFI_STAGE_RECV_GETMODEDATA = 301;
static constexpr unsigned int EFI_STAGE_RECEIVE = 302;
static constexpr unsigned int EFI_STAGE_XMIT_GETMODEDATA = 303;
static constexpr unsigned int EFI_STAGE_TRANSMIT = 304;
static constexpr unsigned int EFI_STAGE_GET_TCP4 = 310;
static constexpr unsigned int EFI_STAGE_CONNECT = 311;
static constexpr unsigned int EFI_STAGE_ACCEPT = 312;

static volatile unsigned int gMainStage = 0;
static volatile unsigned int gMainStageDetail = 0;
static volatile unsigned int gTickStage = 0;
static volatile unsigned long long gTickProcIterations = 0;
static volatile unsigned long long gMainLoopIterations = 0;
