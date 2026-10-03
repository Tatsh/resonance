#include "msg/advancesectionmsg.h"
#include "msg/advancesectiontogglemsg.h"
#include "msg/allnotesoffmsg.h"
#include "msg/autocatchmsg.h"
#include "msg/axebuttonmsg.h"
#include "msg/axisfxmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/axisxpowmsg.h"
#include "msg/axisypowmsg.h"
#include "msg/barstatusmsg.h"
#include "msg/begingamelocalmsg.h"
#include "msg/beginphrasecatchmsg.h"
#include "msg/bsloadlevelpacket.h"
#include "msg/bumpmsg.h"
#include "msg/bumppacket.h"
#include "msg/buttonpowmsg.h"
#include "msg/catchmsg.h"
#include "msg/catchprogresspacket.h"
#include "msg/caughtbarmsg.h"
#include "msg/caughtphrasepacket.h"
#include "msg/caughtpowerbarmsg.h"
#include "msg/choosepowerupmsg.h"
#include "msg/cleargemmsg.h"
#include "msg/cleargemsmsg.h"
#include "msg/contctrlmsg.h"
#include "msg/cripplemsg.h"
#include "msg/cripplepacket.h"
#include "msg/csclientstatuspacket.h"
#include "msg/csinitiateplaypacket.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/displaypointermsg.h"
#include "msg/durgemmsg.h"
#include "msg/enablefreestylemsg.h"
#include "msg/endgamemsg.h"
#include "msg/erasemsg.h"
#include "msg/eraseoffmsg.h"
#include "msg/fadegamemsg.h"
#include "msg/freestylefxmsg.h"
#include "msg/gamebeginmsg.h"
#include "msg/gamechatpacket.h"
#include "msg/gameconnectfailuremsg.h"
#include "msg/gameconnectionlostmsg.h"
#include "msg/gameconnectsuccessmsg.h"
#include "msg/gamemanagerdoplaybackmsg.h"
#include "msg/gameovermsg.h"
#include "msg/gemmsg.h"
#include "msg/gempacket.h"
#include "msg/invalidateseekermsg.h"
#include "msg/invalidatetrackmsg.h"
#include "msg/isrecordingmsg.h"
#include "msg/jameffectmsg.h"
#include "msg/juiceamountmsg.h"
#include "msg/leavegamemsg.h"
#include "msg/lobbyconnectionlostmsg.h"
#include "msg/looptogglemsg.h"
#include "msg/looptoolmsg.h"
#include "msg/messagefactory.h"
#include "msg/metfreqendedmsg.h"
#include "msg/metstartnetlaunchmsg.h"
#include "msg/metstartpausemsg.h"
#include "msg/metunlockstagesmsg.h"
#include "msg/multimusemsg.h"
#include "msg/multipliermsg.h"
#include "msg/multiplierstatemsg.h"
#include "msg/nearesttrackmsg.h"
#include "msg/neutralizemsg.h"
#include "msg/notemsg.h"
#include "msg/nowbarmsg.h"
#include "msg/pausegamesystemmsg.h"
#include "msg/phrasecapturedmsg.h"
#include "msg/phrasemsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/phrasepacket.h"
#include "msg/pitchmsg.h"
#include "msg/pitchriffmsg.h"
#include "msg/playbackmodemsg.h"
#include "msg/playbacktogglemsg.h"
#include "msg/playerleftgamemsg.h"
#include "msg/playerstrackneutralizedmsg.h"
#include "msg/pointamountmsg.h"
#include "msg/powerupcountmsg.h"
#include "msg/powerupfailedmsg.h"
#include "msg/psjoinrequestpacket.h"
#include "msg/rawcontrollermsg.h"
#include "msg/refreshnetmsg.h"
#include "msg/remixfxmsg.h"
#include "msg/remotetrackselectmsg.h"
#include "msg/rotleftmsg.h"
#include "msg/rotrightmsg.h"
#include "msg/scallclientsstatuspacket.h"
#include "msg/scallplayersinfopacket.h"
#include "msg/scgameoverpacket.h"
#include "msg/scloadlevelpacket.h"
#include "msg/scplayerjoinedpacket.h"
#include "msg/scriptmsg.h"
#include "msg/scstartplayingpacket.h"
#include "msg/sectioncapturedmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/spjoinacceptpacket.h"
#include "msg/spjoindenypacket.h"
#include "msg/stdmidimsg.h"
#include "msg/stopriffmsg.h"
#include "msg/streakovermsg.h"
#include "msg/susgemmsg.h"
#include "msg/sustainnotemsg.h"
#include "msg/testarbiterpacket.h"
#include "msg/textmsg.h"
#include "msg/toggleghostmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/trackselectpacket.h"
#include "msg/tracksonmsg.h"
#include "msg/unpausegamesystemmsg.h"
#include "msg/updatescorepacket.h"
#include "msg/winmsg.h"

// Message and packet type identities. Every concrete message reports one of these from its
// Type() method, and the factory constructs by the same identity. The declarations are in the
// headers of their message classes under src/msg/.

// NTSC-U/C: 0x006d01d4, PAL: 0x0071396c
unsigned int g_dwAllNotesOffMsgType = 203;
// NTSC-U/C: 0x006d01dc, PAL: 0x00713974
unsigned int g_dwMultiMuseMsgType = 204;
// NTSC-U/C: 0x006d01cc, PAL: 0x00713964
unsigned int g_dwNoteMsgType = 202;
// NTSC-U/C: 0x006d01c4, PAL: 0x0071395c
unsigned int g_dwStdMidiMsgType = 201;
// NTSC-U/C: 0x006d01e4, PAL: 0x0071397c
unsigned int g_dwSustainNoteMsgType = 205;
// NTSC-U/C: 0x006d01ec, PAL: 0x00713984
unsigned int g_dwTrackSelectMsgType = 300;
// NTSC-U/C: 0x006d0294, PAL: 0x00713a2c
unsigned int g_dwTracksOnMsgType = 321;
// NTSC-U/C: 0x006d0184, PAL: 0x0071391c
int g_nAdvanceSectionMsgType = 113;
// NTSC-U/C: 0x006d025c, PAL: 0x007139f4
int g_nAdvanceSectionToggleMsgType = 314;
// NTSC-U/C: 0x006d03cc, PAL: 0x00713b64
int g_nAutoCatchMsgType = 500;
// NTSC-U/C: 0x006d0304, PAL: 0x00713a9c
int g_nAxeButtonMsgType = 412;
// NTSC-U/C: 0x006d0154, PAL: 0x007138ec
int g_nAxisFXMsgType = 107;
// NTSC-U/C: 0x006d014c, PAL: 0x007138e4
int g_nAxisRegisterMsgType = 106;
// NTSC-U/C: 0x006d0164, PAL: 0x007138fc
int g_nAxisXPowMsgType = 109;
// NTSC-U/C: 0x006d015c, PAL: 0x007138f4
int g_nAxisYPowMsgType = 108;
// NTSC-U/C: 0x006d73b4, PAL: 0x0071ab54
int g_nBSLoadLevelPacketType = 607;
// NTSC-U/C: 0x006d02b4, PAL: 0x00713a4c
int g_nBarStatusMsgType = 402;
// NTSC-U/C: 0x006d03a4, PAL: 0x00713b3c
int g_nBeginGameLocalMsgType = 432;
// NTSC-U/C: 0x006d028c, PAL: 0x00713a24
int g_nBeginPhraseCatchMsgType = 320;
// NTSC-U/C: 0x006d0374, PAL: 0x00713b0c
int g_nBumpMsgType = 426;
// NTSC-U/C: 0x006d7404, PAL: 0x0071aba4
int g_nBumpPacketType = 618;
// NTSC-U/C: 0x006d016c, PAL: 0x00713904
int g_nButtonPowMsgType = 110;
// NTSC-U/C: 0x006d738c, PAL: 0x0071ab2c
int g_nCSClientStatusPacketType = 604;
// NTSC-U/C: 0x006d739c, PAL: 0x0071ab3c
int g_nCSInitiatePlayPacketType = 609;
// NTSC-U/C: 0x006d02e4, PAL: 0x00713a7c
int g_nCatchMsgType = 408;
// NTSC-U/C: 0x006d73f4, PAL: 0x0071ab94
int g_nCatchProgressPacketType = 617;
// NTSC-U/C: 0x006d01b4, PAL: 0x0071394c
int g_nCaughtBarMsgType = 119;
// NTSC-U/C: 0x006d73dc, PAL: 0x0071ab7c
int g_nCaughtPhrasePacketType = 614;
// NTSC-U/C: 0x006d0204, PAL: 0x0071399c
int g_nCaughtPowerbarMsgType = 303;
// NTSC-U/C: 0x006d020c, PAL: 0x007139a4
int g_nChoosePowerupMsgType = 304;
// NTSC-U/C: 0x006d02c4, PAL: 0x00713a5c
int g_nClearGemMsgType = 404;
// NTSC-U/C: 0x006d02bc, PAL: 0x00713a54
int g_nClearGemsMsgType = 403;
// NTSC-U/C: 0x006d02f4, PAL: 0x00713a8c
int g_nContCtrlMsgType = 410;
// NTSC-U/C: 0x006d03d4, PAL: 0x00713b6c
int g_nCrippleMsgType = 501;
// NTSC-U/C: 0x006d73fc, PAL: 0x0071ab9c
int g_nCripplePacketType = 619;
// NTSC-U/C: 0x006d021c, PAL: 0x007139b4
int g_nDeployedPowerupMsgType = 306;
// NTSC-U/C: 0x006d023c, PAL: 0x007139d4
int g_nDisplayPointerMsgType = 310;
// NTSC-U/C: 0x006d02d4, PAL: 0x00713a6c
int g_nDurGemMsgType = 406;
// NTSC-U/C: 0x006d0314, PAL: 0x00713aac
int g_nEnableFreestyleMsgType = 414;
// NTSC-U/C: 0x006d03ac, PAL: 0x00713b44
int g_nEndGameMsgType = 433;
// NTSC-U/C: 0x006d01c0, PAL: 0x00713958
int g_nEndMuseMsgType = 206;
// NTSC-U/C: 0x006d0174, PAL: 0x0071390c
int g_nEraseMsgType = 111;
// NTSC-U/C: 0x006d017c, PAL: 0x00713914
int g_nEraseOffMsgType = 112;
// NTSC-U/C: 0x006d0364, PAL: 0x00713afc
int g_nFadeGameMsgType = 424;
// NTSC-U/C: 0x006d01bc, PAL: 0x00713954
int g_nFirstMuseMsgType = 200;
// NTSC-U/C: 0x006d029c, PAL: 0x00713a34
int g_nFreestyleFXMsgType = 322;
// NTSC-U/C: 0x006d0344, PAL: 0x00713adc
int g_nGameBeginMsgType = 420;
// NTSC-U/C: 0x006d740c, PAL: 0x0071abac
int g_nGameChatPacketType = 620;
// NTSC-U/C: 0x006d0384, PAL: 0x00713b1c
int g_nGameConnectFailureMsgType = 428;
// NTSC-U/C: 0x006d037c, PAL: 0x00713b14
int g_nGameConnectSuccessMsgType = 427;
// NTSC-U/C: 0x006d038c, PAL: 0x00713b24
int g_nGameConnectionLostMsgType = 429;
// NTSC-U/C: 0x006d03c4, PAL: 0x00713b5c
int g_nGameManagerDoPlaybackMsgType = 436;
// NTSC-U/C: 0x006d0354, PAL: 0x00713aec
int g_nGameOverMsgType = 422;
// NTSC-U/C: 0x006d02cc, PAL: 0x00713a64
int g_nGemMsgType = 405;
// NTSC-U/C: 0x006d73cc, PAL: 0x0071ab6c
int g_nGemPacketType = 612;
// NTSC-U/C: 0x006d0274, PAL: 0x00713a0c
int g_nInvalidateSeekerMsgType = 317;
// NTSC-U/C: 0x006d027c, PAL: 0x00713a14
int g_nInvalidateTrackMsgType = 318;
// NTSC-U/C: 0x006d03ec, PAL: 0x00713b84
int g_nIsRecordingMsgType = 504;
// NTSC-U/C: 0x006d032c, PAL: 0x00713ac4
int g_nJamEffectMsgType = 417;
// NTSC-U/C: 0x006d0334, PAL: 0x00713acc
int g_nJuiceAmountMsgType = 418;
// NTSC-U/C: 0x006d035c, PAL: 0x00713af4
int g_nLeaveGameMsgType = 423;
// NTSC-U/C: 0x006d0394, PAL: 0x00713b2c
int g_nLobbyConnectionLostMsgType = 430;
// NTSC-U/C: 0x006d0254, PAL: 0x007139ec
int g_nLoopToggleMsgType = 313;
// NTSC-U/C: 0x006d018c, PAL: 0x00713924
int g_nLoopToolMsgType = 114;
// NTSC-U/C: 0x006d03f4, PAL: 0x00713b8c
int g_nMetFreqEndedMsgType = 505;
// NTSC-U/C: 0x006d03dc, PAL: 0x00713b74
int g_nMetStartNetLaunchMsgType = 502;
// NTSC-U/C: 0x006d03e4, PAL: 0x00713b7c
int g_nMetStartPauseMsgType = 503;
// NTSC-U/C: 0x006d03fc, PAL: 0x00713b94
int g_nMetUnlockStagesMsgType = 506;
// NTSC-U/C: 0x006d019c, PAL: 0x00713934
int g_nMultiplierMsgType = 116;
// NTSC-U/C: 0x006d01a4, PAL: 0x0071393c
int g_nMultiplierStateMsgType = 117;
// NTSC-U/C: 0x006d0234, PAL: 0x007139cc
int g_nNearestTrackMsgType = 309;
// NTSC-U/C: 0x006d031c, PAL: 0x00713ab4
int g_nNeutralizeMsgType = 415;
// NTSC-U/C: 0x006d02fc, PAL: 0x00713a94
int g_nNowBarMsgType = 411;
// NTSC-U/C: 0x006d7368, PAL: 0x0071ab08
int g_nPSJoinRequestPacketType = 600;
// NTSC-U/C: 0x006d03b4, PAL: 0x00713b4c
int g_nPauseGameSystemMsgType = 434;
// NTSC-U/C: 0x006d02a4, PAL: 0x00713a3c
int g_nPhraseCapturedMsgType = 400;
// NTSC-U/C: 0x006d036c, PAL: 0x00713b04
int g_nPhraseMsgType = 425;
// NTSC-U/C: 0x006d030c, PAL: 0x00713aa4
int g_nPhraseMuffedMsgType = 413;
// NTSC-U/C: 0x006d73d4, PAL: 0x0071ab74
int g_nPhrasePacketType = 613;
// NTSC-U/C: 0x006d02ec, PAL: 0x00713a84
int g_nPitchMsgType = 409;
// NTSC-U/C: 0x006d0134, PAL: 0x007138cc
int g_nPitchRiffMsgType = 103;
// NTSC-U/C: 0x006d0144, PAL: 0x007138dc
int g_nPlaybackModeMsgType = 105;
// NTSC-U/C: 0x006d026c, PAL: 0x00713a04
int g_nPlaybackToggleMsgType = 316;
// NTSC-U/C: 0x006d039c, PAL: 0x00713b34
int g_nPlayerLeftGameMsgType = 431;
// NTSC-U/C: 0x006d0324, PAL: 0x00713abc
int g_nPlayersTrackNeutralizedMsgType = 416;
// NTSC-U/C: 0x006d033c, PAL: 0x00713ad4
int g_nPointAmountMsgType = 419;
// NTSC-U/C: 0x006d0214, PAL: 0x007139ac
int g_nPowerupCountMsgType = 305;
// NTSC-U/C: 0x006d0224, PAL: 0x007139bc
int g_nPowerupFailedMsgType = 307;
// NTSC-U/C: 0x006d0118, PAL: 0x007138b0
int g_nRawControllerMsgType = 100;
// NTSC-U/C: 0x006d0284, PAL: 0x00713a1c
int g_nRefreshNetMsgType = 319;
// NTSC-U/C: 0x006d022c, PAL: 0x007139c4
int g_nRemixFXMsgType = 308;
// NTSC-U/C: 0x006d01f4, PAL: 0x0071398c
int g_nRemoteTrackSelectMsgType = 301;
// NTSC-U/C: 0x006d0124, PAL: 0x007138bc
int g_nRotLeftMsgType = 101;
// NTSC-U/C: 0x006d012c, PAL: 0x007138c4
int g_nRotRightMsgType = 102;
// NTSC-U/C: 0x006d7394, PAL: 0x0071ab34
int g_nSCAllClientsStatusPacketType = 605;
// NTSC-U/C: 0x006d73a4, PAL: 0x0071ab44
int g_nSCAllPlayersInfoPacketType = 606;
// NTSC-U/C: 0x006d73c4, PAL: 0x0071ab64
int g_nSCGameOverPacketType = 611;
// NTSC-U/C: 0x006d73bc, PAL: 0x0071ab5c
int g_nSCLoadLevelPacketType = 608;
// NTSC-U/C: 0x006d7384, PAL: 0x0071ab24
int g_nSCPlayerJoinedPacketType = 603;
// NTSC-U/C: 0x006d73ac, PAL: 0x0071ab4c
int g_nSCStartPlayingPacketType = 610;
// NTSC-U/C: 0x006d7374, PAL: 0x0071ab14
int g_nSPJoinAcceptPacketType = 601;
// NTSC-U/C: 0x006d737c, PAL: 0x0071ab1c
int g_nSPJoinDenyPacketType = 602;
// NTSC-U/C: 0x006d024c, PAL: 0x007139e4
int g_nScriptMsgType = 312;
// NTSC-U/C: 0x006d02ac, PAL: 0x00713a44
int g_nSectionCapturedMsgType = 401;
// NTSC-U/C: 0x006d01fc, PAL: 0x00713994
int g_nSeekerMsgType = 302;
// NTSC-U/C: 0x006d0264, PAL: 0x007139fc
int g_nShowEraseEffectMsgType = 315;
// NTSC-U/C: 0x006d013c, PAL: 0x007138d4
int g_nStopRiffMsgType = 104;
// NTSC-U/C: 0x006d01ac, PAL: 0x00713944
int g_nStreakOverMsgType = 118;
// NTSC-U/C: 0x006d02dc, PAL: 0x00713a74
int g_nSusGemMsgType = 407;
// NTSC-U/C: 0x006d7414, PAL: 0x0071abb4
int g_nTestArbiterPacketType = 621;
// NTSC-U/C: 0x006d0244, PAL: 0x007139dc
int g_nTextMsgType = 311;
// NTSC-U/C: 0x006d0194, PAL: 0x0071392c
int g_nToggleGhostMsgType = 115;
// NTSC-U/C: 0x006d73ec, PAL: 0x0071ab8c
int g_nTrackSelectPacketType = 616;
// NTSC-U/C: 0x006d03bc, PAL: 0x00713b54
int g_nUnpauseGameSystemMsgType = 435;
// NTSC-U/C: 0x006d73e4, PAL: 0x0071ab84
int g_nUpdateScorePacketType = 615;
// NTSC-U/C: 0x006d034c, PAL: 0x00713ae4
int g_nWinMsgType = 421;

namespace {

// The static initialiser at 0x003d9818 constructs the message registrars in this order.
// NTSC-U/C: 0x006d0120, PAL: 0x007138b8
const MessageFactory kRawControllerMsgFactory(g_nRawControllerMsgType, RawControllerMsg::New);
// NTSC-U/C: 0x006d0128, PAL: 0x007138c0
const MessageFactory kRotLeftMsgFactory(g_nRotLeftMsgType, RotLeftMsg::New);
// NTSC-U/C: 0x006d0130, PAL: 0x007138c8
const MessageFactory kRotRightMsgFactory(g_nRotRightMsgType, RotRightMsg::New);
// NTSC-U/C: 0x006d0138, PAL: 0x007138d0
const MessageFactory kPitchRiffMsgFactory(g_nPitchRiffMsgType, PitchRiffMsg::New);
// NTSC-U/C: 0x006d0140, PAL: 0x007138d8
const MessageFactory kStopRiffMsgFactory(g_nStopRiffMsgType, StopRiffMsg::New);
// NTSC-U/C: 0x006d0148, PAL: 0x007138e0
const MessageFactory kPlaybackModeMsgFactory(g_nPlaybackModeMsgType, PlaybackModeMsg::New);
// NTSC-U/C: 0x006d0150, PAL: 0x007138e8
const MessageFactory kAxisRegisterMsgFactory(g_nAxisRegisterMsgType, AxisRegisterMsg::New);
// NTSC-U/C: 0x006d0158, PAL: 0x007138f0
const MessageFactory kAxisFXMsgFactory(g_nAxisFXMsgType, AxisFXMsg::New);
// NTSC-U/C: 0x006d0160, PAL: 0x007138f8
const MessageFactory kAxisYPowMsgFactory(g_nAxisYPowMsgType, AxisYPowMsg::New);
// NTSC-U/C: 0x006d0168, PAL: 0x00713900
const MessageFactory kAxisXPowMsgFactory(g_nAxisXPowMsgType, AxisXPowMsg::New);
// NTSC-U/C: 0x006d0170, PAL: 0x00713908
const MessageFactory kButtonPowMsgFactory(g_nButtonPowMsgType, ButtonPowMsg::New);
// NTSC-U/C: 0x006d0178, PAL: 0x00713910
const MessageFactory kEraseMsgFactory(g_nEraseMsgType, EraseMsg::New);
// NTSC-U/C: 0x006d0180, PAL: 0x00713918
const MessageFactory kEraseOffMsgFactory(g_nEraseOffMsgType, EraseOffMsg::New);
// NTSC-U/C: 0x006d0188, PAL: 0x00713920
const MessageFactory kAdvanceSectionMsgFactory(g_nAdvanceSectionMsgType, AdvanceSectionMsg::New);
// NTSC-U/C: 0x006d0190, PAL: 0x00713928
const MessageFactory kLoopToolMsgFactory(g_nLoopToolMsgType, LoopToolMsg::New);
// NTSC-U/C: 0x006d0198, PAL: 0x00713930
const MessageFactory kToggleGhostMsgFactory(g_nToggleGhostMsgType, ToggleGhostMsg::New);
// NTSC-U/C: 0x006d01a0, PAL: 0x00713938
const MessageFactory kMultiplierMsgFactory(g_nMultiplierMsgType, MultiplierMsg::New);
// NTSC-U/C: 0x006d01a8, PAL: 0x00713940
const MessageFactory kMultiplierStateMsgFactory(g_nMultiplierStateMsgType, MultiplierStateMsg::New);
// NTSC-U/C: 0x006d01b0, PAL: 0x00713948
const MessageFactory kStreakOverMsgFactory(g_nStreakOverMsgType, StreakOverMsg::New);
// NTSC-U/C: 0x006d01b8, PAL: 0x00713950
const MessageFactory kCaughtBarMsgFactory(g_nCaughtBarMsgType, CaughtBarMsg::New);
// NTSC-U/C: 0x006d01c8, PAL: 0x00713960
const MessageFactory kStdMidiMsgFactory(g_dwStdMidiMsgType, StdMidiMsg::New);
// NTSC-U/C: 0x006d01d0, PAL: 0x00713968
const MessageFactory kNoteMsgFactory(g_dwNoteMsgType, NoteMsg::New);
// NTSC-U/C: 0x006d01d8, PAL: 0x00713970
const MessageFactory kAllNotesOffMsgFactory(g_dwAllNotesOffMsgType, AllNotesOffMsg::New);
// NTSC-U/C: 0x006d01e0, PAL: 0x00713978
const MessageFactory kMultiMuseMsgFactory(g_dwMultiMuseMsgType, MultiMuseMsg::New);
// NTSC-U/C: 0x006d01e8, PAL: 0x00713980
const MessageFactory kSustainNoteMsgFactory(g_dwSustainNoteMsgType, SustainNoteMsg::New);
// NTSC-U/C: 0x006d01f0, PAL: 0x00713988
const MessageFactory kTrackSelectMsgFactory(g_dwTrackSelectMsgType, TrackSelectMsg::New);
// NTSC-U/C: 0x006d01f8, PAL: 0x00713990
const MessageFactory kRemoteTrackSelectMsgFactory(g_nRemoteTrackSelectMsgType,
                                                  RemoteTrackSelectMsg::New);
// NTSC-U/C: 0x006d0200, PAL: 0x00713998
const MessageFactory kSeekerMsgFactory(g_nSeekerMsgType, SeekerMsg::New);
// NTSC-U/C: 0x006d0208, PAL: 0x007139a0
const MessageFactory kCaughtPowerbarMsgFactory(g_nCaughtPowerbarMsgType, CaughtPowerbarMsg::New);
// NTSC-U/C: 0x006d0210, PAL: 0x007139a8
const MessageFactory kChoosePowerupMsgFactory(g_nChoosePowerupMsgType, ChoosePowerupMsg::New);
// NTSC-U/C: 0x006d0218, PAL: 0x007139b0
const MessageFactory kPowerupCountMsgFactory(g_nPowerupCountMsgType, PowerupCountMsg::New);
// NTSC-U/C: 0x006d0220, PAL: 0x007139b8
const MessageFactory kDeployedPowerupMsgFactory(g_nDeployedPowerupMsgType, DeployedPowerupMsg::New);
// NTSC-U/C: 0x006d0228, PAL: 0x007139c0
const MessageFactory kPowerupFailedMsgFactory(g_nPowerupFailedMsgType, PowerupFailedMsg::New);
// NTSC-U/C: 0x006d0230, PAL: 0x007139c8
const MessageFactory kRemixFXMsgFactory(g_nRemixFXMsgType, RemixFXMsg::New);
// NTSC-U/C: 0x006d0238, PAL: 0x007139d0
const MessageFactory kNearestTrackMsgFactory(g_nNearestTrackMsgType, NearestTrackMsg::New);
// NTSC-U/C: 0x006d0240, PAL: 0x007139d8
const MessageFactory kDisplayPointerMsgFactory(g_nDisplayPointerMsgType, DisplayPointerMsg::New);
// NTSC-U/C: 0x006d0248, PAL: 0x007139e0
const MessageFactory kTextMsgFactory(g_nTextMsgType, TextMsg::New);
// NTSC-U/C: 0x006d0250, PAL: 0x007139e8
const MessageFactory kScriptMsgFactory(g_nScriptMsgType, ScriptMsg::New);
// NTSC-U/C: 0x006d0258, PAL: 0x007139f0
const MessageFactory kLoopToggleMsgFactory(g_nLoopToggleMsgType, LoopToggleMsg::New);
// NTSC-U/C: 0x006d0260, PAL: 0x007139f8
const MessageFactory kAdvanceSectionToggleMsgFactory(g_nAdvanceSectionToggleMsgType,
                                                     AdvanceSectionToggleMsg::New);
// NTSC-U/C: 0x006d0268, PAL: 0x00713a00
const MessageFactory kShowEraseEffectMsgFactory(g_nShowEraseEffectMsgType, ShowEraseEffectMsg::New);
// NTSC-U/C: 0x006d0270, PAL: 0x00713a08
const MessageFactory kPlaybackToggleMsgFactory(g_nPlaybackToggleMsgType, PlaybackToggleMsg::New);
// NTSC-U/C: 0x006d0278, PAL: 0x00713a10
const MessageFactory kInvalidateSeekerMsgFactory(g_nInvalidateSeekerMsgType,
                                                 InvalidateSeekerMsg::New);
// NTSC-U/C: 0x006d0280, PAL: 0x00713a18
const MessageFactory kInvalidateTrackMsgFactory(g_nInvalidateTrackMsgType, InvalidateTrackMsg::New);
// NTSC-U/C: 0x006d0288, PAL: 0x00713a20
const MessageFactory kRefreshNetMsgFactory(g_nRefreshNetMsgType, RefreshNetMsg::New);
// NTSC-U/C: 0x006d0290, PAL: 0x00713a28
const MessageFactory kBeginPhraseCatchMsgFactory(g_nBeginPhraseCatchMsgType,
                                                 BeginPhraseCatchMsg::New);
// NTSC-U/C: 0x006d0298, PAL: 0x00713a30
const MessageFactory kTracksOnMsgFactory(g_dwTracksOnMsgType, TracksOnMsg::New);
// NTSC-U/C: 0x006d02a0, PAL: 0x00713a38
const MessageFactory kFreestyleFXMsgFactory(g_nFreestyleFXMsgType, FreestyleFXMsg::New);
// NTSC-U/C: 0x006d02a8, PAL: 0x00713a40
const MessageFactory kPhraseCapturedMsgFactory(g_nPhraseCapturedMsgType, PhraseCapturedMsg::New);
// NTSC-U/C: 0x006d02b0, PAL: 0x00713a48
const MessageFactory kSectionCapturedMsgFactory(g_nSectionCapturedMsgType, SectionCapturedMsg::New);
// NTSC-U/C: 0x006d02b8, PAL: 0x00713a50
const MessageFactory kBarStatusMsgFactory(g_nBarStatusMsgType, BarStatusMsg::New);
// NTSC-U/C: 0x006d02c0, PAL: 0x00713a58
const MessageFactory kClearGemsMsgFactory(g_nClearGemsMsgType, ClearGemsMsg::New);
// NTSC-U/C: 0x006d02c8, PAL: 0x00713a60
const MessageFactory kClearGemMsgFactory(g_nClearGemMsgType, ClearGemMsg::New);
// NTSC-U/C: 0x006d02d0, PAL: 0x00713a68
const MessageFactory kGemMsgFactory(g_nGemMsgType, GemMsg::New);
// NTSC-U/C: 0x006d02d8, PAL: 0x00713a70
const MessageFactory kDurGemMsgFactory(g_nDurGemMsgType, DurGemMsg::New);
// NTSC-U/C: 0x006d02e0, PAL: 0x00713a78
const MessageFactory kSusGemMsgFactory(g_nSusGemMsgType, SusGemMsg::New);
// NTSC-U/C: 0x006d02e8, PAL: 0x00713a80
const MessageFactory kCatchMsgFactory(g_nCatchMsgType, CatchMsg::New);
// NTSC-U/C: 0x006d02f0, PAL: 0x00713a88
const MessageFactory kPitchMsgFactory(g_nPitchMsgType, PitchMsg::New);
// NTSC-U/C: 0x006d02f8, PAL: 0x00713a90
const MessageFactory kContCtrlMsgFactory(g_nContCtrlMsgType, ContCtrlMsg::New);
// NTSC-U/C: 0x006d0300, PAL: 0x00713a98
const MessageFactory kNowBarMsgFactory(g_nNowBarMsgType, NowBarMsg::New);
// NTSC-U/C: 0x006d0308, PAL: 0x00713aa0
const MessageFactory kAxeButtonMsgFactory(g_nAxeButtonMsgType, AxeButtonMsg::New);
// NTSC-U/C: 0x006d0310, PAL: 0x00713aa8
const MessageFactory kPhraseMuffedMsgFactory(g_nPhraseMuffedMsgType, PhraseMuffedMsg::New);
// NTSC-U/C: 0x006d0318, PAL: 0x00713ab0
const MessageFactory kEnableFreestyleMsgFactory(g_nEnableFreestyleMsgType, EnableFreestyleMsg::New);
// NTSC-U/C: 0x006d0320, PAL: 0x00713ab8
const MessageFactory kNeutralizeMsgFactory(g_nNeutralizeMsgType, NeutralizeMsg::New);
// NTSC-U/C: 0x006d0328, PAL: 0x00713ac0
const MessageFactory kPlayersTrackNeutralizedMsgFactory(g_nPlayersTrackNeutralizedMsgType,
                                                        PlayersTrackNeutralizedMsg::New);
// NTSC-U/C: 0x006d0330, PAL: 0x00713ac8
const MessageFactory kJamEffectMsgFactory(g_nJamEffectMsgType, JamEffectMsg::New);
// NTSC-U/C: 0x006d0338, PAL: 0x00713ad0
const MessageFactory kJuiceAmountMsgFactory(g_nJuiceAmountMsgType, JuiceAmountMsg::New);
// NTSC-U/C: 0x006d0340, PAL: 0x00713ad8
const MessageFactory kPointAmountMsgFactory(g_nPointAmountMsgType, PointAmountMsg::New);
// NTSC-U/C: 0x006d0348, PAL: 0x00713ae0
const MessageFactory kGameBeginMsgFactory(g_nGameBeginMsgType, GameBeginMsg::New);
// NTSC-U/C: 0x006d0350, PAL: 0x00713ae8
const MessageFactory kWinMsgFactory(g_nWinMsgType, WinMsg::New);
// NTSC-U/C: 0x006d0358, PAL: 0x00713af0
const MessageFactory kGameOverMsgFactory(g_nGameOverMsgType, GameOverMsg::New);
// NTSC-U/C: 0x006d0360, PAL: 0x00713af8
const MessageFactory kLeaveGameMsgFactory(g_nLeaveGameMsgType, LeaveGameMsg::New);
// NTSC-U/C: 0x006d0368, PAL: 0x00713b00
const MessageFactory kFadeGameMsgFactory(g_nFadeGameMsgType, FadeGameMsg::New);
// NTSC-U/C: 0x006d0370, PAL: 0x00713b08
const MessageFactory kPhraseMsgFactory(g_nPhraseMsgType, PhraseMsg::New);
// NTSC-U/C: 0x006d0378, PAL: 0x00713b10
const MessageFactory kBumpMsgFactory(g_nBumpMsgType, BumpMsg::New);
// NTSC-U/C: 0x006d0380, PAL: 0x00713b18
const MessageFactory kGameConnectSuccessMsgFactory(g_nGameConnectSuccessMsgType,
                                                   GameConnectSuccessMsg::New);
// NTSC-U/C: 0x006d0388, PAL: 0x00713b20
const MessageFactory kGameConnectFailureMsgFactory(g_nGameConnectFailureMsgType,
                                                   GameConnectFailureMsg::New);
// NTSC-U/C: 0x006d0390, PAL: 0x00713b28
const MessageFactory kGameConnectionLostMsgFactory(g_nGameConnectionLostMsgType,
                                                   GameConnectionLostMsg::New);
// NTSC-U/C: 0x006d0398, PAL: 0x00713b30
const MessageFactory kLobbyConnectionLostMsgFactory(g_nLobbyConnectionLostMsgType,
                                                    LobbyConnectionLostMsg::New);
// NTSC-U/C: 0x006d03a0, PAL: 0x00713b38
const MessageFactory kPlayerLeftGameMsgFactory(g_nPlayerLeftGameMsgType, PlayerLeftGameMsg::New);
// NTSC-U/C: 0x006d03a8, PAL: 0x00713b40
const MessageFactory kBeginGameLocalMsgFactory(g_nBeginGameLocalMsgType, BeginGameLocalMsg::New);
// NTSC-U/C: 0x006d03b0, PAL: 0x00713b48
const MessageFactory kEndGameMsgFactory(g_nEndGameMsgType, EndGameMsg::New);
// NTSC-U/C: 0x006d03b8, PAL: 0x00713b50
const MessageFactory kPauseGameSystemMsgFactory(g_nPauseGameSystemMsgType, PauseGameSystemMsg::New);
// NTSC-U/C: 0x006d03c0, PAL: 0x00713b58
const MessageFactory kUnpauseGameSystemMsgFactory(g_nUnpauseGameSystemMsgType,
                                                  UnpauseGameSystemMsg::New);
// NTSC-U/C: 0x006d03c8, PAL: 0x00713b60
const MessageFactory kGameManagerDoPlaybackMsgFactory(g_nGameManagerDoPlaybackMsgType,
                                                      GameManagerDoPlaybackMsg::New);
// NTSC-U/C: 0x006d03d0, PAL: 0x00713b68
const MessageFactory kAutoCatchMsgFactory(g_nAutoCatchMsgType, AutoCatchMsg::New);
// NTSC-U/C: 0x006d03d8, PAL: 0x00713b70
const MessageFactory kCrippleMsgFactory(g_nCrippleMsgType, CrippleMsg::New);
// NTSC-U/C: 0x006d03e0, PAL: 0x00713b78
const MessageFactory kMetStartNetLaunchMsgFactory(g_nMetStartNetLaunchMsgType,
                                                  MetStartNetLaunchMsg::New);
// NTSC-U/C: 0x006d03e8, PAL: 0x00713b80
const MessageFactory kMetStartPauseMsgFactory(g_nMetStartPauseMsgType, MetStartPauseMsg::New);
// NTSC-U/C: 0x006d03f0, PAL: 0x00713b88
const MessageFactory kIsRecordingMsgFactory(g_nIsRecordingMsgType, IsRecordingMsg::New);
// NTSC-U/C: 0x006d03f8, PAL: 0x00713b90
const MessageFactory kMetFreqEndedMsgFactory(g_nMetFreqEndedMsgType, MetFreqEndedMsg::New);
// NTSC-U/C: 0x006d0400, PAL: 0x00713b98
const MessageFactory kMetUnlockStagesMsgFactory(g_nMetUnlockStagesMsgType, MetUnlockStagesMsg::New);

// The static initialiser at 0x003ed2e0 constructs the packet registrars in this order.
// NTSC-U/C: 0x006d7370, PAL: 0x0071ab10
const MessageFactory kPSJoinRequestPacketFactory(g_nPSJoinRequestPacketType,
                                                 PSJoinRequestPacket::New);
// NTSC-U/C: 0x006d7378, PAL: 0x0071ab18
const MessageFactory kSPJoinAcceptPacketFactory(g_nSPJoinAcceptPacketType, SPJoinAcceptPacket::New);
// NTSC-U/C: 0x006d7380, PAL: 0x0071ab20
const MessageFactory kSPJoinDenyPacketFactory(g_nSPJoinDenyPacketType, SPJoinDenyPacket::New);
// NTSC-U/C: 0x006d7388, PAL: 0x0071ab28
const MessageFactory kSCPlayerJoinedPacketFactory(g_nSCPlayerJoinedPacketType,
                                                  SCPlayerJoinedPacket::New);
// NTSC-U/C: 0x006d7390, PAL: 0x0071ab30
const MessageFactory kCSClientStatusPacketFactory(g_nCSClientStatusPacketType,
                                                  CSClientStatusPacket::New);
// NTSC-U/C: 0x006d7398, PAL: 0x0071ab38
const MessageFactory kSCAllClientsStatusPacketFactory(g_nSCAllClientsStatusPacketType,
                                                      SCAllClientsStatusPacket::New);
// NTSC-U/C: 0x006d73a0, PAL: 0x0071ab40
const MessageFactory kCSInitiatePlayPacketFactory(g_nCSInitiatePlayPacketType,
                                                  CSInitiatePlayPacket::New);
// NTSC-U/C: 0x006d73a8, PAL: 0x0071ab48
const MessageFactory kSCAllPlayersInfoPacketFactory(g_nSCAllPlayersInfoPacketType,
                                                    SCAllPlayersInfoPacket::New);
// NTSC-U/C: 0x006d73b0, PAL: 0x0071ab50
const MessageFactory kSCStartPlayingPacketFactory(g_nSCStartPlayingPacketType,
                                                  SCStartPlayingPacket::New);
// NTSC-U/C: 0x006d73b8, PAL: 0x0071ab58
const MessageFactory kBSLoadLevelPacketFactory(g_nBSLoadLevelPacketType, BSLoadLevelPacket::New);
// NTSC-U/C: 0x006d73c0, PAL: 0x0071ab60
const MessageFactory kSCLoadLevelPacketFactory(g_nSCLoadLevelPacketType, SCLoadLevelPacket::New);
// NTSC-U/C: 0x006d73c8, PAL: 0x0071ab68
const MessageFactory kSCGameOverPacketFactory(g_nSCGameOverPacketType, SCGameOverPacket::New);
// NTSC-U/C: 0x006d73d0, PAL: 0x0071ab70
const MessageFactory kGemPacketFactory(g_nGemPacketType, GemPacket::New);
// NTSC-U/C: 0x006d73d8, PAL: 0x0071ab78
const MessageFactory kPhrasePacketFactory(g_nPhrasePacketType, PhrasePacket::New);
// NTSC-U/C: 0x006d73e0, PAL: 0x0071ab80
const MessageFactory kCaughtPhrasePacketFactory(g_nCaughtPhrasePacketType, CaughtPhrasePacket::New);
// NTSC-U/C: 0x006d73e8, PAL: 0x0071ab88
const MessageFactory kUpdateScorePacketFactory(g_nUpdateScorePacketType, UpdateScorePacket::New);
// NTSC-U/C: 0x006d73f0, PAL: 0x0071ab90
const MessageFactory kTrackSelectPacketFactory(g_nTrackSelectPacketType, TrackSelectPacket::New);
// NTSC-U/C: 0x006d73f8, PAL: 0x0071ab98
const MessageFactory kCatchProgressPacketFactory(g_nCatchProgressPacketType,
                                                 CatchProgressPacket::New);
// NTSC-U/C: 0x006d7400, PAL: 0x0071aba0
const MessageFactory kCripplePacketFactory(g_nCripplePacketType, CripplePacket::New);
// NTSC-U/C: 0x006d7408, PAL: 0x0071aba8
const MessageFactory kBumpPacketFactory(g_nBumpPacketType, BumpPacket::New);
// NTSC-U/C: 0x006d7410, PAL: 0x0071abb0
const MessageFactory kGameChatPacketFactory(g_nGameChatPacketType, GameChatPacket::New);
// NTSC-U/C: 0x006d7418, PAL: 0x0071abb8
const MessageFactory kTestArbiterPacketFactory(g_nTestArbiterPacketType, TestArbiterPacket::New);

} // namespace
