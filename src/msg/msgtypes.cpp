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

// 0x006d01d4
unsigned int g_dwAllNotesOffMsgType = 203;
// 0x006d01dc
unsigned int g_dwMultiMuseMsgType = 204;
// 0x006d01cc
unsigned int g_dwNoteMsgType = 202;
// 0x006d01c4
unsigned int g_dwStdMidiMsgType = 201;
// 0x006d01e4
unsigned int g_dwSustainNoteMsgType = 205;
// 0x006d01ec
unsigned int g_dwTrackSelectMsgType = 300;
// 0x006d0294
unsigned int g_dwTracksOnMsgType = 321;
// 0x006d0184
int g_nAdvanceSectionMsgType = 113;
// 0x006d025c
int g_nAdvanceSectionToggleMsgType = 314;
// 0x006d03cc
int g_nAutoCatchMsgType = 500;
// 0x006d0304
int g_nAxeButtonMsgType = 412;
// 0x006d0154
int g_nAxisFXMsgType = 107;
// 0x006d014c
int g_nAxisRegisterMsgType = 106;
// 0x006d0164
int g_nAxisXPowMsgType = 109;
// 0x006d015c
int g_nAxisYPowMsgType = 108;
// 0x006d73b4
int g_nBSLoadLevelPacketType = 607;
// 0x006d02b4
int g_nBarStatusMsgType = 402;
// 0x006d03a4
int g_nBeginGameLocalMsgType = 432;
// 0x006d028c
int g_nBeginPhraseCatchMsgType = 320;
// 0x006d0374
int g_nBumpMsgType = 426;
// 0x006d7404
int g_nBumpPacketType = 618;
// 0x006d016c
int g_nButtonPowMsgType = 110;
// 0x006d738c
int g_nCSClientStatusPacketType = 604;
// 0x006d739c
int g_nCSInitiatePlayPacketType = 609;
// 0x006d02e4
int g_nCatchMsgType = 408;
// 0x006d73f4
int g_nCatchProgressPacketType = 617;
// 0x006d01b4
int g_nCaughtBarMsgType = 119;
// 0x006d73dc
int g_nCaughtPhrasePacketType = 614;
// 0x006d0204
int g_nCaughtPowerbarMsgType = 303;
// 0x006d020c
int g_nChoosePowerupMsgType = 304;
// 0x006d02c4
int g_nClearGemMsgType = 404;
// 0x006d02bc
int g_nClearGemsMsgType = 403;
// 0x006d02f4
int g_nContCtrlMsgType = 410;
// 0x006d03d4
int g_nCrippleMsgType = 501;
// 0x006d73fc
int g_nCripplePacketType = 619;
// 0x006d021c
int g_nDeployedPowerupMsgType = 306;
// 0x006d023c
int g_nDisplayPointerMsgType = 310;
// 0x006d02d4
int g_nDurGemMsgType = 406;
// 0x006d0314
int g_nEnableFreestyleMsgType = 414;
// 0x006d03ac
int g_nEndGameMsgType = 433;
// 0x006d01c0
int g_nEndMuseMsgType = 206;
// 0x006d0174
int g_nEraseMsgType = 111;
// 0x006d017c
int g_nEraseOffMsgType = 112;
// 0x006d0364
int g_nFadeGameMsgType = 424;
// 0x006d01bc
int g_nFirstMuseMsgType = 200;
// 0x006d029c
int g_nFreestyleFXMsgType = 322;
// 0x006d0344
int g_nGameBeginMsgType = 420;
// 0x006d740c
int g_nGameChatPacketType = 620;
// 0x006d0384
int g_nGameConnectFailureMsgType = 428;
// 0x006d037c
int g_nGameConnectSuccessMsgType = 427;
// 0x006d038c
int g_nGameConnectionLostMsgType = 429;
// 0x006d03c4
int g_nGameManagerDoPlaybackMsgType = 436;
// 0x006d0354
int g_nGameOverMsgType = 422;
// 0x006d02cc
int g_nGemMsgType = 405;
// 0x006d73cc
int g_nGemPacketType = 612;
// 0x006d0274
int g_nInvalidateSeekerMsgType = 317;
// 0x006d027c
int g_nInvalidateTrackMsgType = 318;
// 0x006d03ec
int g_nIsRecordingMsgType = 504;
// 0x006d032c
int g_nJamEffectMsgType = 417;
// 0x006d0334
int g_nJuiceAmountMsgType = 418;
// 0x006d035c
int g_nLeaveGameMsgType = 423;
// 0x006d0394
int g_nLobbyConnectionLostMsgType = 430;
// 0x006d0254
int g_nLoopToggleMsgType = 313;
// 0x006d018c
int g_nLoopToolMsgType = 114;
// 0x006d03f4
int g_nMetFreqEndedMsgType = 505;
// 0x006d03dc
int g_nMetStartNetLaunchMsgType = 502;
// 0x006d03e4
int g_nMetStartPauseMsgType = 503;
// 0x006d03fc
int g_nMetUnlockStagesMsgType = 506;
// 0x006d019c
int g_nMultiplierMsgType = 116;
// 0x006d01a4
int g_nMultiplierStateMsgType = 117;
// 0x006d0234
int g_nNearestTrackMsgType = 309;
// 0x006d031c
int g_nNeutralizeMsgType = 415;
// 0x006d02fc
int g_nNowBarMsgType = 411;
// 0x006d7368
int g_nPSJoinRequestPacketType = 600;
// 0x006d03b4
int g_nPauseGameSystemMsgType = 434;
// 0x006d02a4
int g_nPhraseCapturedMsgType = 400;
// 0x006d036c
int g_nPhraseMsgType = 425;
// 0x006d030c
int g_nPhraseMuffedMsgType = 413;
// 0x006d73d4
int g_nPhrasePacketType = 613;
// 0x006d02ec
int g_nPitchMsgType = 409;
// 0x006d0134
int g_nPitchRiffMsgType = 103;
// 0x006d0144
int g_nPlaybackModeMsgType = 105;
// 0x006d026c
int g_nPlaybackToggleMsgType = 316;
// 0x006d039c
int g_nPlayerLeftGameMsgType = 431;
// 0x006d0324
int g_nPlayersTrackNeutralizedMsgType = 416;
// 0x006d033c
int g_nPointAmountMsgType = 419;
// 0x006d0214
int g_nPowerupCountMsgType = 305;
// 0x006d0224
int g_nPowerupFailedMsgType = 307;
// 0x006d0118
int g_nRawControllerMsgType = 100;
// 0x006d0284
int g_nRefreshNetMsgType = 319;
// 0x006d022c
int g_nRemixFXMsgType = 308;
// 0x006d01f4
int g_nRemoteTrackSelectMsgType = 301;
// 0x006d0124
int g_nRotLeftMsgType = 101;
// 0x006d012c
int g_nRotRightMsgType = 102;
// 0x006d7394
int g_nSCAllClientsStatusPacketType = 605;
// 0x006d73a4
int g_nSCAllPlayersInfoPacketType = 606;
// 0x006d73c4
int g_nSCGameOverPacketType = 611;
// 0x006d73bc
int g_nSCLoadLevelPacketType = 608;
// 0x006d7384
int g_nSCPlayerJoinedPacketType = 603;
// 0x006d73ac
int g_nSCStartPlayingPacketType = 610;
// 0x006d7374
int g_nSPJoinAcceptPacketType = 601;
// 0x006d737c
int g_nSPJoinDenyPacketType = 602;
// 0x006d024c
int g_nScriptMsgType = 312;
// 0x006d02ac
int g_nSectionCapturedMsgType = 401;
// 0x006d01fc
int g_nSeekerMsgType = 302;
// 0x006d0264
int g_nShowEraseEffectMsgType = 315;
// 0x006d013c
int g_nStopRiffMsgType = 104;
// 0x006d01ac
int g_nStreakOverMsgType = 118;
// 0x006d02dc
int g_nSusGemMsgType = 407;
// 0x006d7414
int g_nTestArbiterPacketType = 621;
// 0x006d0244
int g_nTextMsgType = 311;
// 0x006d0194
int g_nToggleGhostMsgType = 115;
// 0x006d73ec
int g_nTrackSelectPacketType = 616;
// 0x006d03bc
int g_nUnpauseGameSystemMsgType = 435;
// 0x006d73e4
int g_nUpdateScorePacketType = 615;
// 0x006d034c
int g_nWinMsgType = 421;

namespace {

// The static initialiser at 0x003d9818 constructs the message registrars in this order.
// 0x006d0120
const MessageFactory kRawControllerMsgFactory(g_nRawControllerMsgType, RawControllerMsg::New);
// 0x006d0128
const MessageFactory kRotLeftMsgFactory(g_nRotLeftMsgType, RotLeftMsg::New);
// 0x006d0130
const MessageFactory kRotRightMsgFactory(g_nRotRightMsgType, RotRightMsg::New);
// 0x006d0138
const MessageFactory kPitchRiffMsgFactory(g_nPitchRiffMsgType, PitchRiffMsg::New);
// 0x006d0140
const MessageFactory kStopRiffMsgFactory(g_nStopRiffMsgType, StopRiffMsg::New);
// 0x006d0148
const MessageFactory kPlaybackModeMsgFactory(g_nPlaybackModeMsgType, PlaybackModeMsg::New);
// 0x006d0150
const MessageFactory kAxisRegisterMsgFactory(g_nAxisRegisterMsgType, AxisRegisterMsg::New);
// 0x006d0158
const MessageFactory kAxisFXMsgFactory(g_nAxisFXMsgType, AxisFXMsg::New);
// 0x006d0160
const MessageFactory kAxisYPowMsgFactory(g_nAxisYPowMsgType, AxisYPowMsg::New);
// 0x006d0168
const MessageFactory kAxisXPowMsgFactory(g_nAxisXPowMsgType, AxisXPowMsg::New);
// 0x006d0170
const MessageFactory kButtonPowMsgFactory(g_nButtonPowMsgType, ButtonPowMsg::New);
// 0x006d0178
const MessageFactory kEraseMsgFactory(g_nEraseMsgType, EraseMsg::New);
// 0x006d0180
const MessageFactory kEraseOffMsgFactory(g_nEraseOffMsgType, EraseOffMsg::New);
// 0x006d0188
const MessageFactory kAdvanceSectionMsgFactory(g_nAdvanceSectionMsgType, AdvanceSectionMsg::New);
// 0x006d0190
const MessageFactory kLoopToolMsgFactory(g_nLoopToolMsgType, LoopToolMsg::New);
// 0x006d0198
const MessageFactory kToggleGhostMsgFactory(g_nToggleGhostMsgType, ToggleGhostMsg::New);
// 0x006d01a0
const MessageFactory kMultiplierMsgFactory(g_nMultiplierMsgType, MultiplierMsg::New);
// 0x006d01a8
const MessageFactory kMultiplierStateMsgFactory(g_nMultiplierStateMsgType, MultiplierStateMsg::New);
// 0x006d01b0
const MessageFactory kStreakOverMsgFactory(g_nStreakOverMsgType, StreakOverMsg::New);
// 0x006d01b8
const MessageFactory kCaughtBarMsgFactory(g_nCaughtBarMsgType, CaughtBarMsg::New);
// 0x006d01c8
const MessageFactory kStdMidiMsgFactory(g_dwStdMidiMsgType, StdMidiMsg::New);
// 0x006d01d0
const MessageFactory kNoteMsgFactory(g_dwNoteMsgType, NoteMsg::New);
// 0x006d01d8
const MessageFactory kAllNotesOffMsgFactory(g_dwAllNotesOffMsgType, AllNotesOffMsg::New);
// 0x006d01e0
const MessageFactory kMultiMuseMsgFactory(g_dwMultiMuseMsgType, MultiMuseMsg::New);
// 0x006d01e8
const MessageFactory kSustainNoteMsgFactory(g_dwSustainNoteMsgType, SustainNoteMsg::New);
// 0x006d01f0
const MessageFactory kTrackSelectMsgFactory(g_dwTrackSelectMsgType, TrackSelectMsg::New);
// 0x006d01f8
const MessageFactory kRemoteTrackSelectMsgFactory(g_nRemoteTrackSelectMsgType,
                                                  RemoteTrackSelectMsg::New);
// 0x006d0200
const MessageFactory kSeekerMsgFactory(g_nSeekerMsgType, SeekerMsg::New);
// 0x006d0208
const MessageFactory kCaughtPowerbarMsgFactory(g_nCaughtPowerbarMsgType, CaughtPowerbarMsg::New);
// 0x006d0210
const MessageFactory kChoosePowerupMsgFactory(g_nChoosePowerupMsgType, ChoosePowerupMsg::New);
// 0x006d0218
const MessageFactory kPowerupCountMsgFactory(g_nPowerupCountMsgType, PowerupCountMsg::New);
// 0x006d0220
const MessageFactory kDeployedPowerupMsgFactory(g_nDeployedPowerupMsgType, DeployedPowerupMsg::New);
// 0x006d0228
const MessageFactory kPowerupFailedMsgFactory(g_nPowerupFailedMsgType, PowerupFailedMsg::New);
// 0x006d0230
const MessageFactory kRemixFXMsgFactory(g_nRemixFXMsgType, RemixFXMsg::New);
// 0x006d0238
const MessageFactory kNearestTrackMsgFactory(g_nNearestTrackMsgType, NearestTrackMsg::New);
// 0x006d0240
const MessageFactory kDisplayPointerMsgFactory(g_nDisplayPointerMsgType, DisplayPointerMsg::New);
// 0x006d0248
const MessageFactory kTextMsgFactory(g_nTextMsgType, TextMsg::New);
// 0x006d0250
const MessageFactory kScriptMsgFactory(g_nScriptMsgType, ScriptMsg::New);
// 0x006d0258
const MessageFactory kLoopToggleMsgFactory(g_nLoopToggleMsgType, LoopToggleMsg::New);
// 0x006d0260
const MessageFactory kAdvanceSectionToggleMsgFactory(g_nAdvanceSectionToggleMsgType,
                                                     AdvanceSectionToggleMsg::New);
// 0x006d0268
const MessageFactory kShowEraseEffectMsgFactory(g_nShowEraseEffectMsgType, ShowEraseEffectMsg::New);
// 0x006d0270
const MessageFactory kPlaybackToggleMsgFactory(g_nPlaybackToggleMsgType, PlaybackToggleMsg::New);
// 0x006d0278
const MessageFactory kInvalidateSeekerMsgFactory(g_nInvalidateSeekerMsgType,
                                                 InvalidateSeekerMsg::New);
// 0x006d0280
const MessageFactory kInvalidateTrackMsgFactory(g_nInvalidateTrackMsgType, InvalidateTrackMsg::New);
// 0x006d0288
const MessageFactory kRefreshNetMsgFactory(g_nRefreshNetMsgType, RefreshNetMsg::New);
// 0x006d0290
const MessageFactory kBeginPhraseCatchMsgFactory(g_nBeginPhraseCatchMsgType,
                                                 BeginPhraseCatchMsg::New);
// 0x006d0298
const MessageFactory kTracksOnMsgFactory(g_dwTracksOnMsgType, TracksOnMsg::New);
// 0x006d02a0
const MessageFactory kFreestyleFXMsgFactory(g_nFreestyleFXMsgType, FreestyleFXMsg::New);
// 0x006d02a8
const MessageFactory kPhraseCapturedMsgFactory(g_nPhraseCapturedMsgType, PhraseCapturedMsg::New);
// 0x006d02b0
const MessageFactory kSectionCapturedMsgFactory(g_nSectionCapturedMsgType, SectionCapturedMsg::New);
// 0x006d02b8
const MessageFactory kBarStatusMsgFactory(g_nBarStatusMsgType, BarStatusMsg::New);
// 0x006d02c0
const MessageFactory kClearGemsMsgFactory(g_nClearGemsMsgType, ClearGemsMsg::New);
// 0x006d02c8
const MessageFactory kClearGemMsgFactory(g_nClearGemMsgType, ClearGemMsg::New);
// 0x006d02d0
const MessageFactory kGemMsgFactory(g_nGemMsgType, GemMsg::New);
// 0x006d02d8
const MessageFactory kDurGemMsgFactory(g_nDurGemMsgType, DurGemMsg::New);
// 0x006d02e0
const MessageFactory kSusGemMsgFactory(g_nSusGemMsgType, SusGemMsg::New);
// 0x006d02e8
const MessageFactory kCatchMsgFactory(g_nCatchMsgType, CatchMsg::New);
// 0x006d02f0
const MessageFactory kPitchMsgFactory(g_nPitchMsgType, PitchMsg::New);
// 0x006d02f8
const MessageFactory kContCtrlMsgFactory(g_nContCtrlMsgType, ContCtrlMsg::New);
// 0x006d0300
const MessageFactory kNowBarMsgFactory(g_nNowBarMsgType, NowBarMsg::New);
// 0x006d0308
const MessageFactory kAxeButtonMsgFactory(g_nAxeButtonMsgType, AxeButtonMsg::New);
// 0x006d0310
const MessageFactory kPhraseMuffedMsgFactory(g_nPhraseMuffedMsgType, PhraseMuffedMsg::New);
// 0x006d0318
const MessageFactory kEnableFreestyleMsgFactory(g_nEnableFreestyleMsgType, EnableFreestyleMsg::New);
// 0x006d0320
const MessageFactory kNeutralizeMsgFactory(g_nNeutralizeMsgType, NeutralizeMsg::New);
// 0x006d0328
const MessageFactory kPlayersTrackNeutralizedMsgFactory(g_nPlayersTrackNeutralizedMsgType,
                                                        PlayersTrackNeutralizedMsg::New);
// 0x006d0330
const MessageFactory kJamEffectMsgFactory(g_nJamEffectMsgType, JamEffectMsg::New);
// 0x006d0338
const MessageFactory kJuiceAmountMsgFactory(g_nJuiceAmountMsgType, JuiceAmountMsg::New);
// 0x006d0340
const MessageFactory kPointAmountMsgFactory(g_nPointAmountMsgType, PointAmountMsg::New);
// 0x006d0348
const MessageFactory kGameBeginMsgFactory(g_nGameBeginMsgType, GameBeginMsg::New);
// 0x006d0350
const MessageFactory kWinMsgFactory(g_nWinMsgType, WinMsg::New);
// 0x006d0358
const MessageFactory kGameOverMsgFactory(g_nGameOverMsgType, GameOverMsg::New);
// 0x006d0360
const MessageFactory kLeaveGameMsgFactory(g_nLeaveGameMsgType, LeaveGameMsg::New);
// 0x006d0368
const MessageFactory kFadeGameMsgFactory(g_nFadeGameMsgType, FadeGameMsg::New);
// 0x006d0370
const MessageFactory kPhraseMsgFactory(g_nPhraseMsgType, PhraseMsg::New);
// 0x006d0378
const MessageFactory kBumpMsgFactory(g_nBumpMsgType, BumpMsg::New);
// 0x006d0380
const MessageFactory kGameConnectSuccessMsgFactory(g_nGameConnectSuccessMsgType,
                                                   GameConnectSuccessMsg::New);
// 0x006d0388
const MessageFactory kGameConnectFailureMsgFactory(g_nGameConnectFailureMsgType,
                                                   GameConnectFailureMsg::New);
// 0x006d0390
const MessageFactory kGameConnectionLostMsgFactory(g_nGameConnectionLostMsgType,
                                                   GameConnectionLostMsg::New);
// 0x006d0398
const MessageFactory kLobbyConnectionLostMsgFactory(g_nLobbyConnectionLostMsgType,
                                                    LobbyConnectionLostMsg::New);
// 0x006d03a0
const MessageFactory kPlayerLeftGameMsgFactory(g_nPlayerLeftGameMsgType, PlayerLeftGameMsg::New);
// 0x006d03a8
const MessageFactory kBeginGameLocalMsgFactory(g_nBeginGameLocalMsgType, BeginGameLocalMsg::New);
// 0x006d03b0
const MessageFactory kEndGameMsgFactory(g_nEndGameMsgType, EndGameMsg::New);
// 0x006d03b8
const MessageFactory kPauseGameSystemMsgFactory(g_nPauseGameSystemMsgType, PauseGameSystemMsg::New);
// 0x006d03c0
const MessageFactory kUnpauseGameSystemMsgFactory(g_nUnpauseGameSystemMsgType,
                                                  UnpauseGameSystemMsg::New);
// 0x006d03c8
const MessageFactory kGameManagerDoPlaybackMsgFactory(g_nGameManagerDoPlaybackMsgType,
                                                      GameManagerDoPlaybackMsg::New);
// 0x006d03d0
const MessageFactory kAutoCatchMsgFactory(g_nAutoCatchMsgType, AutoCatchMsg::New);
// 0x006d03d8
const MessageFactory kCrippleMsgFactory(g_nCrippleMsgType, CrippleMsg::New);
// 0x006d03e0
const MessageFactory kMetStartNetLaunchMsgFactory(g_nMetStartNetLaunchMsgType,
                                                  MetStartNetLaunchMsg::New);
// 0x006d03e8
const MessageFactory kMetStartPauseMsgFactory(g_nMetStartPauseMsgType, MetStartPauseMsg::New);
// 0x006d03f0
const MessageFactory kIsRecordingMsgFactory(g_nIsRecordingMsgType, IsRecordingMsg::New);
// 0x006d03f8
const MessageFactory kMetFreqEndedMsgFactory(g_nMetFreqEndedMsgType, MetFreqEndedMsg::New);
// 0x006d0400
const MessageFactory kMetUnlockStagesMsgFactory(g_nMetUnlockStagesMsgType, MetUnlockStagesMsg::New);

// The static initialiser at 0x003ed2e0 constructs the packet registrars in this order.
// 0x006d7370
const MessageFactory kPSJoinRequestPacketFactory(g_nPSJoinRequestPacketType,
                                                 PSJoinRequestPacket::New);
// 0x006d7378
const MessageFactory kSPJoinAcceptPacketFactory(g_nSPJoinAcceptPacketType, SPJoinAcceptPacket::New);
// 0x006d7380
const MessageFactory kSPJoinDenyPacketFactory(g_nSPJoinDenyPacketType, SPJoinDenyPacket::New);
// 0x006d7388
const MessageFactory kSCPlayerJoinedPacketFactory(g_nSCPlayerJoinedPacketType,
                                                  SCPlayerJoinedPacket::New);
// 0x006d7390
const MessageFactory kCSClientStatusPacketFactory(g_nCSClientStatusPacketType,
                                                  CSClientStatusPacket::New);
// 0x006d7398
const MessageFactory kSCAllClientsStatusPacketFactory(g_nSCAllClientsStatusPacketType,
                                                      SCAllClientsStatusPacket::New);
// 0x006d73a0
const MessageFactory kCSInitiatePlayPacketFactory(g_nCSInitiatePlayPacketType,
                                                  CSInitiatePlayPacket::New);
// 0x006d73a8
const MessageFactory kSCAllPlayersInfoPacketFactory(g_nSCAllPlayersInfoPacketType,
                                                    SCAllPlayersInfoPacket::New);
// 0x006d73b0
const MessageFactory kSCStartPlayingPacketFactory(g_nSCStartPlayingPacketType,
                                                  SCStartPlayingPacket::New);
// 0x006d73b8
const MessageFactory kBSLoadLevelPacketFactory(g_nBSLoadLevelPacketType, BSLoadLevelPacket::New);
// 0x006d73c0
const MessageFactory kSCLoadLevelPacketFactory(g_nSCLoadLevelPacketType, SCLoadLevelPacket::New);
// 0x006d73c8
const MessageFactory kSCGameOverPacketFactory(g_nSCGameOverPacketType, SCGameOverPacket::New);
// 0x006d73d0
const MessageFactory kGemPacketFactory(g_nGemPacketType, GemPacket::New);
// 0x006d73d8
const MessageFactory kPhrasePacketFactory(g_nPhrasePacketType, PhrasePacket::New);
// 0x006d73e0
const MessageFactory kCaughtPhrasePacketFactory(g_nCaughtPhrasePacketType, CaughtPhrasePacket::New);
// 0x006d73e8
const MessageFactory kUpdateScorePacketFactory(g_nUpdateScorePacketType, UpdateScorePacket::New);
// 0x006d73f0
const MessageFactory kTrackSelectPacketFactory(g_nTrackSelectPacketType, TrackSelectPacket::New);
// 0x006d73f8
const MessageFactory kCatchProgressPacketFactory(g_nCatchProgressPacketType,
                                                 CatchProgressPacket::New);
// 0x006d7400
const MessageFactory kCripplePacketFactory(g_nCripplePacketType, CripplePacket::New);
// 0x006d7408
const MessageFactory kBumpPacketFactory(g_nBumpPacketType, BumpPacket::New);
// 0x006d7410
const MessageFactory kGameChatPacketFactory(g_nGameChatPacketType, GameChatPacket::New);
// 0x006d7418
const MessageFactory kTestArbiterPacketFactory(g_nTestArbiterPacketType, TestArbiterPacket::New);

} // namespace
