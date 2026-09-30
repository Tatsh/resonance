// Message and packet type identities. Every concrete message reports one of these from its
// Type() method, and the factory constructs by the same identity. The declarations are in the
// headers of their message classes under include/msg/.

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
