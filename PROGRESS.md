# Progress

Reconstruction status for FreQuency (PlayStation 2, `SCUS-97125`). The figures compare the address
annotations in this tree against the function list of the disassembler project. Update this file
whenever a subsystem lands.

The function list is a fresh dump from the disassembler bridge. A stored list understates the
denominator as identification advances.

```shell
curl -s 'http://127.0.0.1:8089/list_functions?limit=30000' > .wiswa-ci/freq/funcs.txt
uv run --project recon-tools python .wiswa-ci/freq/coverage_report.py .wiswa-ci/freq/funcs.txt freq-src
```

## Coverage

| Measure                   | Count  |
| ------------------------- | ------ |
| Functions in the program  | 15,647 |
| Excluded by rule          | 8,724  |
| Reconstructable           | 6,923  |
| Declared or defined       | 6,475  |
| Share declared or defined | 93.53% |
| Defined, with a body      | 6,109  |
| Share implemented         | 88.24% |
| Remaining, with a name    | 448    |
| Remaining, unidentified   | 0      |

The table measures a clean export of the commit `d47a35c`.

The audit counts an address as accounted once any file in the tree annotates it, and a header
declaration takes the same annotation a body does. Treat 88.24% as the answer to "how much is
reconstructed" and 93.53% as the answer to "how much is accounted for".

Of the 6,475 accounted routines, 6,109 have a body the scanner counts. Of the other 366, the
great majority are reconstructable routines declared with their address, signature, and evidence
(inline in headers, template instances, split signatures, and defaulted or vendored glue), and the
rest are annotated library and vendored routines whose titles fall outside the body count.

Of the 448 routines with no annotation, 249 are implicit special members, 91 are static
initialiser and exit stubs, 57 are ezmpeg sample routines, 5 are unreferenced interpreter workers
whose entries inline their bodies, 20 are library routines labelled with their upstream names, 11
are script workers (ten unreferenced duplicates of live cheat entry points and one scheduler kill
with no caller), six are SDK routines the open-source SDK provides or links (three
interrupt-context entries, two interrupt toggles, and one stream-input helper), eight are recorded
exceptions, and one is a measurement gap. The canvas factory
`ACanvas::CreateForSubBitmap()` at `0x005e8e38` and the `ABitmap` sub-rectangle constructor at
`0x00558dd8` it calls, found in code the disassembler had not defined, both have bodies in the
tree since the script-layer push.

Of the 448 remaining routines, 204 are compiler-generated destructors: the deleting-destructor
wrappers and the implicit destructors the tree never declares. The completion table below
omits those 204 and tracks the other 244.

### Remaining routines

Every routine below is verified complete. The mark records a body in the tree or a recorded
rule from the gaps list. Addresses are relative to the image base and lengths are byte counts
from the disassembler. Signatures are the prototypes Ghidra reports, with its separator written
as `::` and the calling convention omitted. The reference count unions the disassembler's
references with aligned byte-level call and address matches, covering code and data.
Compiler-generated destructors are excluded from this table: the deleting-destructor
wrappers and the implicit destructors the tree never declares.

| Done | Address      | Length | # xrefs | Name                                             | Preliminary signature                                                                                   |
| ---- | ------------ | ------ | ------- | ------------------------------------------------ | ------------------------------------------------------------------------------------------------------- |
| ❌   | `0x00100350` | 31     | 5       | `Unit00100040::GlobalCtors`                      | `void Unit00100040::GlobalCtors(void)`                                                                  |
| ❌   | `0x0010baa0` | 615    | 77      | `PlayerInfo::ConstructCopy`                      | `void * PlayerInfo::ConstructCopy(void * pOther)`                                                       |
| ❌   | `0x0010c8b0` | 31     | 5       | `Unit0010b530::GlobalCtors`                      | `undefined Unit0010b530::GlobalCtors(void)`                                                             |
| ❌   | `0x0010f130` | 31     | 4       | `Unit0010ed58::GlobalCtors`                      | `undefined Unit0010ed58::GlobalCtors(void)`                                                             |
| ❌   | `0x00115860` | 479    | 2       | `MsgSource::ConstructCopy`                       | `undefined MsgSource::ConstructCopy(void)`                                                              |
| ❌   | `0x00115cc0` | 43     | 37      | `MsgSource::Construct`                           | `undefined MsgSource::Construct(void)`                                                                  |
| ❌   | `0x001170b0` | 31     | 4       | `Unit00115c68::GlobalCtors`                      | `undefined Unit00115c68::GlobalCtors(void)`                                                             |
| ❌   | `0x00119048` | 95     | 1       | `ScriptKillSch`                                  | `PyCxxObject * ScriptKillSch(PyCxxObject * pResult, PyCxxObject * pArgs)`                               |
| ❌   | `0x00119140` | 31     | 4       | `Unit00118960::GlobalCtors`                      | `undefined Unit00118960::GlobalCtors(void)`                                                             |
| ❌   | `0x00122ef0` | 31     | 4       | `Unit00121bf0::GlobalCtors`                      | `undefined Unit00121bf0::GlobalCtors(void)`                                                             |
| ❌   | `0x0012b640` | 31     | 4       | `Unit0012a4a0::GlobalCtors`                      | `undefined Unit0012a4a0::GlobalCtors(void)`                                                             |
| ❌   | `0x0012da80` | 31     | 4       | `Unit0012d178::GlobalCtors`                      | `undefined Unit0012d178::GlobalCtors(void)`                                                             |
| ❌   | `0x0012daa0` | 51     | 1       | `PlayMapRing::Construct`                         | `PlayMapRing * PlayMapRing::Construct(void)`                                                            |
| ❌   | `0x00133538` | 31     | 4       | `Unit00132618::GlobalCtors`                      | `undefined Unit00132618::GlobalCtors(void)`                                                             |
| ❌   | `0x00133558` | 31     | 5       | `Unit00132618::GlobalDtors`                      | `undefined Unit00132618::GlobalDtors(void)`                                                             |
| ❌   | `0x00139530` | 699    | 13      | `RemixIndexElement::ConstructCopy`               | `undefined RemixIndexElement::ConstructCopy(void)`                                                      |
| ❌   | `0x0013ae50` | 31     | 4       | `Unit0013aba0::GlobalCtors`                      | `undefined Unit0013aba0::GlobalCtors(void)`                                                             |
| ❌   | `0x0013b230` | 31     | 4       | `Unit0013af38::GlobalCtors`                      | `undefined Unit0013af38::GlobalCtors(void)`                                                             |
| ❌   | `0x0013faf0` | 31     | 4       | `Unit0013eef8::GlobalCtors`                      | `undefined Unit0013eef8::GlobalCtors(void)`                                                             |
| ❌   | `0x001453e0` | 19     | 2       | `SkillStats::AssignImplicit`                     | `void SkillStats::AssignImplicit(SkillStats * pOther)`                                                  |
| ❌   | `0x0014e848` | 131    | 1       | `ScriptActivatePracticeMode`                     | `PyCxxObject * ScriptActivatePracticeMode(PyCxxObject * pResult, PyCxxObject * pArgs)`                  |
| ❌   | `0x0014e8d0` | 175    | 1       | `ScriptActivateAllAccessMode`                    | `PyCxxObject * ScriptActivateAllAccessMode(PyCxxObject * pResult, PyCxxObject * pArgs)`                 |
| ❌   | `0x0014e980` | 199    | 1       | `ScriptEnableTeamFreqs`                          | `PyCxxObject * ScriptEnableTeamFreqs(PyCxxObject * pResult, PyCxxObject * pArgs)`                       |
| ❌   | `0x0014ea48` | 191    | 1       | `ScriptEnablePowerupCheats`                      | `PyCxxObject * ScriptEnablePowerupCheats(PyCxxObject * pResult, PyCxxObject * pArgs)`                   |
| ❌   | `0x0014eb08` | 87     | 1       | `ScriptDoPowerupCheat`                           | `PyCxxObject * ScriptDoPowerupCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`                        |
| ❌   | `0x0014eb60` | 87     | 1       | `ScriptDoBigGemModeCheat`                        | `PyCxxObject * ScriptDoBigGemModeCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`                     |
| ❌   | `0x0014ebb8` | 87     | 1       | `ScriptDoNoLatticeModeCheat`                     | `PyCxxObject * ScriptDoNoLatticeModeCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`                  |
| ❌   | `0x0014ec60` | 87     | 1       | `ScriptDoArenaStateCycleCheat`                   | `PyCxxObject * ScriptDoArenaStateCycleCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`                |
| ❌   | `0x0014ecb8` | 203    | 1       | `ScriptDoExpansionPackToggleCheat`               | `PyCxxObject * ScriptDoExpansionPackToggleCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`            |
| ❌   | `0x0014ed88` | 91     | 1       | `ScriptDoWinSequenceCheat`                       | `PyCxxObject * ScriptDoWinSequenceCheat(PyCxxObject * pResult, PyCxxObject * pArgs)`                    |
| ❌   | `0x001506c0` | 175    | 1       | `HxScript::ActivateAllAccessMode`                | `undefined HxScript::ActivateAllAccessMode(void)`                                                       |
| ❌   | `0x001508d8` | 31     | 4       | `Unit00150580::StaticCtor`                       | `undefined Unit00150580::StaticCtor(void)`                                                              |
| ❌   | `0x00150cc0` | 167    | 1       | `HxScript::CheatWin`                             | `undefined HxScript::CheatWin(void)`                                                                    |
| ❌   | `0x00150d68` | 31     | 4       | `Unit00150b80::StaticCtor`                       | `undefined Unit00150b80::StaticCtor(void)`                                                              |
| ❌   | `0x00153258` | 31     | 4       | `Unit00153030::StaticCtor`                       | `undefined Unit00153030::StaticCtor(void)`                                                              |
| ❌   | `0x001532b8` | 31     | 4       | `Unit00153278::StaticCtor`                       | `undefined Unit00153278::StaticCtor(void)`                                                              |
| ❌   | `0x001539e0` | 31     | 4       | `Unit001537d8::StaticCtor`                       | `undefined Unit001537d8::StaticCtor(void)`                                                              |
| ❌   | `0x00154238` | 31     | 4       | `Unit00154030::StaticCtor`                       | `undefined Unit00154030::StaticCtor(void)`                                                              |
| ❌   | `0x00154948` | 31     | 4       | `Unit00154740::StaticCtor`                       | `undefined Unit00154740::StaticCtor(void)`                                                              |
| ❌   | `0x00155da0` | 31     | 4       | `Unit00155b98::StaticCtor`                       | `undefined Unit00155b98::StaticCtor(void)`                                                              |
| ❌   | `0x00157558` | 31     | 4       | `Unit00157418::StaticCtor`                       | `undefined Unit00157418::StaticCtor(void)`                                                              |
| ❌   | `0x00159410` | 139    | 1       | `HxScript::StopAllMidi`                          | `undefined HxScript::StopAllMidi(void)`                                                                 |
| ❌   | `0x00159518` | 31     | 4       | `Unit00159248::StaticCtor`                       | `undefined Unit00159248::StaticCtor(void)`                                                              |
| ❌   | `0x0015a800` | 31     | 4       | `Unit0015a1c0::StaticCtor`                       | `undefined Unit0015a1c0::StaticCtor(void)`                                                              |
| ❌   | `0x0015bae0` | 31     | 4       | `Unit0015b850::StaticCtor`                       | `undefined Unit0015b850::StaticCtor(void)`                                                              |
| ❌   | `0x0015c420` | 115    | 1       | `HxScript::Capture`                              | `undefined HxScript::Capture(void)`                                                                     |
| ❌   | `0x0015c560` | 31     | 4       | `Unit0015c2c8::StaticCtor`                       | `undefined Unit0015c2c8::StaticCtor(void)`                                                              |
| ❌   | `0x0015c978` | 31     | 4       | `Unit0015c838::StaticCtor`                       | `undefined Unit0015c838::StaticCtor(void)`                                                              |
| ❌   | `0x0015d930` | 31     | 4       | `Unit0015d6d8::StaticCtor`                       | `undefined Unit0015d6d8::StaticCtor(void)`                                                              |
| ❌   | `0x0015e5d0` | 31     | 4       | `Unit0015e3c8::StaticCtor`                       | `undefined Unit0015e3c8::StaticCtor(void)`                                                              |
| ❌   | `0x0015eb68` | 31     | 4       | `Unit0015e960::StaticCtor`                       | `undefined Unit0015e960::StaticCtor(void)`                                                              |
| ❌   | `0x0015f250` | 31     | 4       | `Unit0015f048::StaticCtor`                       | `undefined Unit0015f048::StaticCtor(void)`                                                              |
| ❌   | `0x0015ff78` | 31     | 4       | `Unit0015fc68::StaticCtor`                       | `undefined Unit0015fc68::StaticCtor(void)`                                                              |
| ❌   | `0x001611d8` | 31     | 4       | `Unit00160fb8::StaticCtor`                       | `undefined Unit00160fb8::StaticCtor(void)`                                                              |
| ❌   | `0x00162c70` | 31     | 4       | `Unit00162a68::StaticCtor`                       | `undefined Unit00162a68::StaticCtor(void)`                                                              |
| ❌   | `0x00163640` | 31     | 4       | `Unit00163438::StaticCtor`                       | `undefined Unit00163438::StaticCtor(void)`                                                              |
| ❌   | `0x00163a10` | 87     | 1       | `HxScript::ZoneDump`                             | `undefined HxScript::ZoneDump(void)`                                                                    |
| ❌   | `0x00163a68` | 31     | 4       | `Unit001638d0::StaticCtor`                       | `undefined Unit001638d0::StaticCtor(void)`                                                              |
| ❌   | `0x00170c88` | 31     | 4       | `Unit0016ff68::GlobalCtors`                      | `void Unit0016ff68::GlobalCtors(void)`                                                                  |
| ❌   | `0x00174da8` | 31     | 4       | `Unit001742f0::GlobalCtors`                      | `undefined Unit001742f0::GlobalCtors(void)`                                                             |
| ❌   | `0x00174dc8` | 31     | 5       | `Unit001742f0::GlobalDtors`                      | `undefined Unit001742f0::GlobalDtors(void)`                                                             |
| ❌   | `0x00183ed0` | 31     | 8       | `RemixIndex::ConstructImplicit`                  | `void * RemixIndex::ConstructImplicit(void)`                                                            |
| ❌   | `0x00184130` | 723    | 39      | `MetRemixRecord::ConstructCopy`                  | `MetRemixRecord * MetRemixRecord::ConstructCopy(MetRemixRecord * pOther)`                               |
| ❌   | `0x00187130` | 31     | 4       | `Unit00187130::StaticCtor`                       | `undefined Unit00187130::StaticCtor(void)`                                                              |
| ❌   | `0x00187150` | 31     | 4       | `Unit00187150::StaticDtor`                       | `undefined Unit00187150::StaticDtor(void)`                                                              |
| ❌   | `0x001940e0` | 63     | 2       | `MsgJoiner::Construct`                           | `void * MsgJoiner::Construct(void)`                                                                     |
| ❌   | `0x0019d1b0` | 111    | 2       | `PhraseMaker::Construct`                         | `PhraseMaker * PhraseMaker::Construct(void)`                                                            |
| ❌   | `0x001a6ed8` | 27     | 1       | `MidiDisabler::UnreferencedSend`                 | `undefined MidiDisabler::UnreferencedSend(void)`                                                        |
| ❌   | `0x001bfbc0` | 51     | 2       | `JamPowerbarMgr::Construct`                      | `JamPowerbarMgr * JamPowerbarMgr::Construct(void)`                                                      |
| ❌   | `0x001c0680` | 51     | 3       | `PowerbarMgr::Construct`                         | `PowerbarMgr * PowerbarMgr::Construct(void)`                                                            |
| ❌   | `0x001de858` | 39     | 2       | `InputCheatDetectorCheatSequence::Construct`     | `InputCheatDetectorCheatSequence * InputCheatDetectorCheatSequence::Construct(void)`                    |
| ❌   | `0x001de880` | 515    | 8       | `InputCheatDetectorCheatSequence::CopyConstruct` | `void InputCheatDetectorCheatSequence::CopyConstruct(InputCheatDetectorCheatSequence * pOther)`         |
| ❌   | `0x001ea1e8` | 27     | 4       | `Harmony::Construct`                             | `Harmony * Harmony::Construct(void)`                                                                    |
| ❌   | `0x001f6530` | 35     | 2       | `MemcardManager::AtExitDestroyShared`            | `void MemcardManager::AtExitDestroyShared(void)`                                                        |
| ❌   | `0x002071b0` | 31     | 4       | `Unit002065a0::GlobalCtors`                      | `undefined Unit002065a0::GlobalCtors(void)`                                                             |
| ❌   | `0x002071d0` | 31     | 4       | `Unit002065a0::GlobalDtors`                      | `undefined Unit002065a0::GlobalDtors(void)`                                                             |
| ❌   | `0x00217ff8` | 127    | 2       | `MetPersonaData::AtExitDestroySavedList`         | `undefined MetPersonaData::AtExitDestroySavedList(void)`                                                |
| ❌   | `0x00218098` | 127    | 2       | `MetPersonaData::AtExitDestroyLoadList`          | `undefined MetPersonaData::AtExitDestroyLoadList(void)`                                                 |
| ❌   | `0x00255750` | 31     | 4       | `Unit00254690::GlobalCtors`                      | `undefined Unit00254690::GlobalCtors(void)`                                                             |
| ❌   | `0x00255770` | 31     | 4       | `Unit00254690::GlobalDtors`                      | `undefined Unit00254690::GlobalDtors(void)`                                                             |
| ❌   | `0x002627d0` | 31     | 4       | `Unit00261d50::GlobalCtors`                      | `undefined Unit00261d50::GlobalCtors(void)`                                                             |
| ❌   | `0x002627f0` | 31     | 4       | `Unit00261d50::GlobalDtors`                      | `undefined Unit00261d50::GlobalDtors(void)`                                                             |
| ❌   | `0x00273120` | 31     | 4       | `Unit00272030::GlobalCtors`                      | `undefined Unit00272030::GlobalCtors(void)`                                                             |
| ❌   | `0x0028ccd8` | 39     | 13      | `MetKeyboardScreen::AtExitDestroyTicker`         | `undefined MetKeyboardScreen::AtExitDestroyTicker(void)`                                                |
| ❌   | `0x0028d288` | 31     | 4       | `Unit0028d288::StaticCtor`                       | `undefined Unit0028d288::StaticCtor(void)`                                                              |
| ❌   | `0x0028d2a8` | 31     | 4       | `Unit0028d2a8::StaticDtor`                       | `undefined Unit0028d2a8::StaticDtor(void)`                                                              |
| ❌   | `0x00344700` | 31     | 4       | `Unit00343848::StaticCtor`                       | `undefined Unit00343848::StaticCtor(void)`                                                              |
| ❌   | `0x00344720` | 31     | 4       | `Unit00343848::StaticDtor`                       | `undefined Unit00343848::StaticDtor(void)`                                                              |
| ❌   | `0x00381d98` | 119    | 2       | `MetScreen::AtExitDestroyContainerLoaderMap`     | `undefined MetScreen::AtExitDestroyContainerLoaderMap(void)`                                            |
| ❌   | `0x00382168` | 119    | 2       | `MetScreen::AtExitDestroyScreenRegistry`         | `undefined MetScreen::AtExitDestroyScreenRegistry(void)`                                                |
| ❌   | `0x003cc678` | 203    | 2       | `LevelNameStorage::StaticDestroy`                | `undefined LevelNameStorage::StaticDestroy(void)`                                                       |
| ❌   | `0x003d6798` | 87     | 1       | `MidMBT::Construct`                              | `MidMBTPolymorphic * MidMBT::Construct(int nTick, int nBeatsPerMeasure, int nTicksPerBeat)`             |
| ❌   | `0x003d67f0` | 115    | 2       | `MidMBT::Print`                                  | `void MidMBT::Print(ostream * stream)`                                                                  |
| ❌   | `0x003e1868` | 43     | 1       | `GameConnectFailureMsg::ConstructCopy`           | `undefined GameConnectFailureMsg::ConstructCopy(void)`                                                  |
| ❌   | `0x003e1ae8` | 43     | 1       | `GameConnectionLostMsg::ConstructCopy`           | `undefined GameConnectionLostMsg::ConstructCopy(void)`                                                  |
| ❌   | `0x003e1d68` | 43     | 1       | `LobbyConnectionLostMsg::ConstructCopy`          | `undefined LobbyConnectionLostMsg::ConstructCopy(void)`                                                 |
| ❌   | `0x003f2dc0` | 135    | 2       | `PSJoinRequestPacket::ConstructCopy`             | `PSJoinRequestPacket * PSJoinRequestPacket::ConstructCopy(PSJoinRequestPacket * pOther)`                |
| ❌   | `0x003f2e48` | 743    | 2       | `SPJoinAcceptPacket::ConstructCopy`              | `SPJoinAcceptPacket * SPJoinAcceptPacket::ConstructCopy(SPJoinAcceptPacket * pOther)`                   |
| ❌   | `0x003f3130` | 147    | 2       | `SPJoinDenyPacket::ConstructCopy`                | `SPJoinDenyPacket * SPJoinDenyPacket::ConstructCopy(SPJoinDenyPacket * pOther)`                         |
| ❌   | `0x003f31c8` | 135    | 2       | `SCPlayerJoinedPacket::ConstructCopy`            | `SCPlayerJoinedPacket * SCPlayerJoinedPacket::ConstructCopy(SCPlayerJoinedPacket * pOther)`             |
| ❌   | `0x003f3250` | 71     | 2       | `CSClientStatusPacket::ConstructCopy`            | `CSClientStatusPacket * CSClientStatusPacket::ConstructCopy(CSClientStatusPacket * pOther)`             |
| ❌   | `0x003f3298` | 523    | 2       | `SCAllClientsStatusPacket::ConstructCopy`        | `SCAllClientsStatusPacket * SCAllClientsStatusPacket::ConstructCopy(SCAllClientsStatusPacket * pOther)` |
| ❌   | `0x003f34a8` | 63     | 2       | `CSInitiatePlayPacket::ConstructCopy`            | `CSInitiatePlayPacket * CSInitiatePlayPacket::ConstructCopy(CSInitiatePlayPacket * pOther)`             |
| ❌   | `0x003f34e8` | 567    | 2       | `SCAllPlayersInfoPacket::ConstructCopy`          | `SCAllPlayersInfoPacket * SCAllPlayersInfoPacket::ConstructCopy(SCAllPlayersInfoPacket * pOther)`       |
| ❌   | `0x003f3720` | 63     | 2       | `SCStartPlayingPacket::ConstructCopy`            | `SCStartPlayingPacket * SCStartPlayingPacket::ConstructCopy(SCStartPlayingPacket * pOther)`             |
| ❌   | `0x003f3760` | 87     | 2       | `PhrasePacket::ConstructCopy`                    | `PhrasePacket * PhrasePacket::ConstructCopy(PhrasePacket * pOther)`                                     |
| ❌   | `0x003f37b8` | 95     | 2       | `CaughtPhrasePacket::ConstructCopy`              | `CaughtPhrasePacket * CaughtPhrasePacket::ConstructCopy(CaughtPhrasePacket * pOther)`                   |
| ❌   | `0x003f3818` | 79     | 2       | `UpdateScorePacket::ConstructCopy`               | `UpdateScorePacket * UpdateScorePacket::ConstructCopy(UpdateScorePacket * pOther)`                      |
| ❌   | `0x003f3868` | 103    | 2       | `TrackSelectPacket::ConstructCopy`               | `TrackSelectPacket * TrackSelectPacket::ConstructCopy(TrackSelectPacket * pOther)`                      |
| ❌   | `0x003f38d0` | 103    | 2       | `CatchProgressPacket::ConstructCopy`             | `CatchProgressPacket * CatchProgressPacket::ConstructCopy(CatchProgressPacket * pOther)`                |
| ❌   | `0x003f3938` | 539    | 2       | `CripplePacket::ConstructCopy`                   | `CripplePacket * CripplePacket::ConstructCopy(CripplePacket * pOther)`                                  |
| ❌   | `0x003f3b58` | 103    | 2       | `BumpPacket::ConstructCopy`                      | `BumpPacket * BumpPacket::ConstructCopy(BumpPacket * pOther)`                                           |
| ❌   | `0x003f3bc0` | 135    | 2       | `BSLoadLevelPacket::ConstructCopy`               | `BSLoadLevelPacket * BSLoadLevelPacket::ConstructCopy(BSLoadLevelPacket * pOther)`                      |
| ❌   | `0x003f3c48` | 135    | 2       | `SCLoadLevelPacket::ConstructCopy`               | `SCLoadLevelPacket * SCLoadLevelPacket::ConstructCopy(SCLoadLevelPacket * pOther)`                      |
| ❌   | `0x003f3cd0` | 71     | 2       | `SCGameOverPacket::ConstructCopy`                | `SCGameOverPacket * SCGameOverPacket::ConstructCopy(SCGameOverPacket * pOther)`                         |
| ❌   | `0x003f3d18` | 111    | 2       | `GemPacket::ConstructCopy`                       | `GemPacket * GemPacket::ConstructCopy(GemPacket * pOther)`                                              |
| ❌   | `0x003f3d88` | 203    | 2       | `GameChatPacket::ConstructCopy`                  | `GameChatPacket * GameChatPacket::ConstructCopy(GameChatPacket * pOther)`                               |
| ❌   | `0x003f3e58` | 203    | 2       | `TestArbiterPacket::ConstructCopy`               | `TestArbiterPacket * TestArbiterPacket::ConstructCopy(TestArbiterPacket * pOther)`                      |
| ❌   | `0x003f7bc0` | 31     | 4       | `Unit003f6760::GlobalCtors`                      | `undefined Unit003f6760::GlobalCtors(void)`                                                             |
| ❌   | `0x003f7be0` | 31     | 4       | `Unit003f6760::GlobalDtors`                      | `undefined Unit003f6760::GlobalDtors(void)`                                                             |
| ❌   | `0x0040cec8` | 31     | 4       | `Unit0040cec8::StaticCtor`                       | `undefined Unit0040cec8::StaticCtor(void)`                                                              |
| ❌   | `0x0040d060` | 111    | 1       | `Delayer::Construct`                             | `undefined Delayer::Construct(void)`                                                                    |
| ❌   | `0x0040d0d0` | 27     | 1       | `Delayer::UnreferencedSend`                      | `undefined Delayer::UnreferencedSend(void)`                                                             |
| ❌   | `0x0042b760` | 31     | 4       | `Unit0042b760::StaticCtor`                       | `void Unit0042b760::StaticCtor(void)`                                                                   |
| ❌   | `0x0042b780` | 31     | 4       | `Unit0042b780::StaticDtor`                       | `void Unit0042b780::StaticDtor(void)`                                                                   |
| ❌   | `0x00432440` | 27     | 1       | `Renderer::UnreferencedSend`                     | `undefined Renderer::UnreferencedSend(void)`                                                            |
| ❌   | `0x004327f0` | 31     | 4       | `Unit004327f0::StaticCtor`                       | `undefined Unit004327f0::StaticCtor(void)`                                                              |
| ❌   | `0x00432810` | 31     | 4       | `Unit00432810::StaticDtor`                       | `undefined Unit00432810::StaticDtor(void)`                                                              |
| ❌   | `0x00461150` | 31     | 4       | `Unit00461150::StaticCtor`                       | `void Unit00461150::StaticCtor(void)`                                                                   |
| ❌   | `0x00461170` | 31     | 4       | `Unit00461170::StaticDtor`                       | `void Unit00461170::StaticDtor(void)`                                                                   |
| ❌   | `0x00464f30` | 31     | 4       | `Unit00464170::StaticCtor`                       | `void Unit00464170::StaticCtor(void)`                                                                   |
| ❌   | `0x00464f50` | 31     | 4       | `Unit00464170::StaticDtor`                       | `void Unit00464170::StaticDtor(void)`                                                                   |
| ❌   | `0x004664e8` | 31     | 4       | `Unit004662d0::StaticCtor`                       | `void Unit004662d0::StaticCtor(void)`                                                                   |
| ❌   | `0x00466508` | 31     | 4       | `Unit004662d0::StaticDtor`                       | `void Unit004662d0::StaticDtor(void)`                                                                   |
| ❌   | `0x00471e28` | 99     | 11      | `TunnelSeekSection::ConstructCopy`               | `undefined TunnelSeekSection::ConstructCopy(void)`                                                      |
| ❌   | `0x00471e90` | 555    | 2       | `TunnelSeekStrip::ConstructCopy`                 | `undefined TunnelSeekStrip::ConstructCopy(void)`                                                        |
| ❌   | `0x004720c0` | 131    | 7       | `TunnelSeeker::CopyConstruct`                    | `TunnelSeeker * TunnelSeeker::CopyConstruct(TunnelSeeker * pOther)`                                     |
| ❌   | `0x004776f8` | 107    | 1       | `TunnelEvent::DrawFiltered`                      | `void TunnelEvent::DrawFiltered(void * pFilter, int nArg)`                                              |
| ❌   | `0x00478808` | 31     | 4       | `Unit00476088::StaticCtor`                       | `undefined Unit00476088::StaticCtor(void)`                                                              |
| ❌   | `0x00478828` | 31     | 4       | `Unit00476088::StaticDtor`                       | `undefined Unit00476088::StaticDtor(void)`                                                              |
| ❌   | `0x00478848` | 47     | 12      | `type_info::Destruct`                            | `undefined type_info::Destruct(void)`                                                                   |
| ❌   | `0x00478d68` | 47     | 3       | `bad_typeid::Destruct`                           | `undefined bad_typeid::Destruct(void)`                                                                  |
| ❌   | `0x00478e00` | 47     | 3       | `bad_cast::Destruct`                             | `undefined bad_cast::Destruct(void)`                                                                    |
| ❌   | `0x00479f98` | 287    | 3       | `ostrstream::Slot1`                              | `undefined ostrstream::Slot1(void)`                                                                     |
| ❌   | `0x0047a118` | 287    | 6       | `istrstream::Slot1`                              | `undefined istrstream::Slot1(void)`                                                                     |
| ❌   | `0x0047e330` | 31     | 4       | `Unit0047dc18::StaticCtor`                       | `undefined Unit0047dc18::StaticCtor(void)`                                                              |
| ❌   | `0x0047e350` | 31     | 4       | `Unit0047dc18::StaticDtor`                       | `undefined Unit0047dc18::StaticDtor(void)`                                                              |
| ❌   | `0x00494a48` | 31     | 4       | `Unit00492340::StaticCtor`                       | `undefined Unit00492340::StaticCtor(void)`                                                              |
| ❌   | `0x00494a68` | 31     | 4       | `Unit00492340::StaticDtor`                       | `undefined Unit00492340::StaticDtor(void)`                                                              |
| ❌   | `0x004a0748` | 31     | 4       | `Unit0049fcc0::StaticCtor`                       | `void Unit0049fcc0::StaticCtor(void)`                                                                   |
| ❌   | `0x004a0768` | 31     | 4       | `Unit0049fcc0::StaticDtor`                       | `void Unit0049fcc0::StaticDtor(void)`                                                                   |
| ❌   | `0x004acdf8` | 31     | 4       | `Unit004ac5c8::StaticCtor`                       | `void Unit004ac5c8::StaticCtor(void)`                                                                   |
| ❌   | `0x004ad5d8` | 47     | 3       | `StdBadException::Destruct`                      | `undefined StdBadException::Destruct(void)`                                                             |
| ❌   | `0x004b2b88` | 31     | 4       | `Unit004b1cb8::StaticCtor`                       | `void Unit004b1cb8::StaticCtor(void)`                                                                   |
| ❌   | `0x004b2ba8` | 31     | 4       | `Unit004b1cb8::StaticDtor`                       | `void Unit004b1cb8::StaticDtor(void)`                                                                   |
| ❌   | `0x004b4660` | 35     | 3       | `Spew::SharedInstanceDtor`                       | `void Spew::SharedInstanceDtor(void)`                                                                   |
| ❌   | `0x004bfd08` | 31     | 4       | `Unit004beb40::StaticCtor`                       | `void Unit004beb40::StaticCtor(void)`                                                                   |
| ❌   | `0x004bfd28` | 31     | 4       | `Unit004beb40::StaticDtor`                       | `void Unit004beb40::StaticDtor(void)`                                                                   |
| ❌   | `0x004c3ba0` | 31     | 4       | `Unit004c32a8::StaticCtor`                       | `void Unit004c32a8::StaticCtor(void)`                                                                   |
| ❌   | `0x004c3bc0` | 31     | 4       | `Unit004c32a8::StaticDtor`                       | `void Unit004c32a8::StaticDtor(void)`                                                                   |
| ❌   | `0x00536eb0` | 15     | 1       | `isceSifSetDma`                                  | `undefined isceSifSetDma(void)`                                                                         |
| ❌   | `0x00536ed0` | 15     | 1       | `isceSifSetDChain`                               | `undefined isceSifSetDChain(void)`                                                                      |
| ❌   | `0x00558db8` | 7      | 1       | `UnreferencedReturnZeroStub00558db8`             | `undefined UnreferencedReturnZeroStub00558db8(void)`                                                    |
| ❌   | `0x00558dc0` | 7      | 1       | `UnreferencedReturnZeroStub00558dc0`             | `undefined UnreferencedReturnZeroStub00558dc0(void)`                                                    |
| ❌   | `0x00558dc8` | 7      | 1       | `UnreferencedReturnZeroStub00558dc8`             | `undefined UnreferencedReturnZeroStub00558dc8(void)`                                                    |
| ❌   | `0x00559830` | 35     | 3       | `HxMethods::StaticDestroy`                       | `void HxMethods::StaticDestroy(void)`                                                                   |
| ❌   | `0x00567670` | 431    | 2       | `audioDecSendToIOP`                              | `undefined audioDecSendToIOP(void)`                                                                     |
| ❌   | `0x00567820` | 187    | 2       | `audioDecCreate`                                 | `undefined audioDecCreate(void)`                                                                        |
| ❌   | `0x005678e0` | 59     | 2       | `audioDecDelete`                                 | `undefined audioDecDelete(void)`                                                                        |
| ❌   | `0x00567a50` | 19     | 2       | `audioDecIsPreset`                               | `undefined audioDecIsPreset(void)`                                                                      |
| ❌   | `0x00567a68` | 107    | 2       | `audioDecStart`                                  | `undefined audioDecStart(void)`                                                                         |
| ❌   | `0x00567ad8` | 163    | 2       | `audioDecReset`                                  | `undefined audioDecReset(void)`                                                                         |
| ❌   | `0x00569128` | 455    | 2       | `decode`                                         | `undefined decode(void)`                                                                                |
| ❌   | `0x005692f0` | 7      | 2       | `MpegDecoderRoutine005692f0`                     | `undefined MpegDecoderRoutine005692f0(void)`                                                            |
| ❌   | `0x005692f8` | 255    | 2       | `videoDecCreate`                                 | `undefined videoDecCreate(void)`                                                                        |
| ❌   | `0x005693f8` | 51     | 2       | `videoDecDelete`                                 | `undefined videoDecDelete(void)`                                                                        |
| ❌   | `0x00569430` | 11     | 2       | `videoDecAbort`                                  | `undefined videoDecAbort(void)`                                                                         |
| ❌   | `0x00569440` | 7      | 3       | `videoDecGetState`                               | `undefined videoDecGetState(void)`                                                                      |
| ❌   | `0x00569458` | 27     | 1       | `videoDecInputCount`                             | `undefined videoDecInputCount(void)`                                                                    |
| ❌   | `0x00569478` | 55     | 1       | `videoDecInputSpaceCount`                        | `undefined videoDecInputSpaceCount(void)`                                                               |
| ❌   | `0x005694b0` | 27     | 1       | `videoDecReset`                                  | `undefined videoDecReset(void)`                                                                         |
| ❌   | `0x005694d0` | 223    | 2       | `videoDecFlush`                                  | `undefined videoDecFlush(void)`                                                                         |
| ❌   | `0x005695b0` | 75     | 2       | `videoDecIsFlushed`                              | `undefined videoDecIsFlushed(void)`                                                                     |
| ❌   | `0x00569600` | 31     | 3       | `videoDecSetStream`                              | `undefined videoDecSetStream(void)`                                                                     |
| ❌   | `0x00569620` | 27     | 2       | `videoDecBeginPut`                               | `undefined videoDecBeginPut(void)`                                                                      |
| ❌   | `0x00569640` | 27     | 2       | `videoDecEndPut`                                 | `undefined videoDecEndPut(void)`                                                                        |
| ❌   | `0x00569660` | 59     | 2       | `videoDecPutTs`                                  | `undefined videoDecPutTs(void)`                                                                         |
| ❌   | `0x005696a0` | 91     | 2       | `videoDecMain`                                   | `undefined videoDecMain(void)`                                                                          |
| ❌   | `0x00569700` | 39     | 2       | `mpegError`                                      | `undefined mpegError(void)`                                                                             |
| ❌   | `0x00569728` | 43     | 2       | `mpegNodata`                                     | `undefined mpegNodata(void)`                                                                            |
| ❌   | `0x00569758` | 35     | 2       | `mpegStopDMA`                                    | `undefined mpegStopDMA(void)`                                                                           |
| ❌   | `0x00569780` | 35     | 2       | `mpegRestartDMA`                                 | `undefined mpegRestartDMA(void)`                                                                        |
| ❌   | `0x005697a8` | 67     | 2       | `mpegTS`                                         | `undefined mpegTS(void)`                                                                                |
| ❌   | `0x005697f0` | 303    | 2       | `cpy2area`                                       | `undefined cpy2area(void)`                                                                              |
| ❌   | `0x0058de70` | 703    | 2       | `strFileOpen`                                    | `undefined strFileOpen(void)`                                                                           |
| ❌   | `0x0058e130` | 79     | 2       | `strFileClose`                                   | `undefined strFileClose(void)`                                                                          |
| ❌   | `0x0058e180` | 59     | 2       | `strFileRead`                                    | `undefined strFileRead(void)`                                                                           |
| ❌   | `0x0059a908` | 223    | 1       | `CheckPalEqual`                                  | `bool CheckPalEqual(APalette * pPalMip0, APalette * pPalMip, char * pszName, int nMip)`                 |
| ❌   | `0x0059afa0` | 291    | 2       | `videoCallback`                                  | `undefined videoCallback(void)`                                                                         |
| ❌   | `0x0059b0c8` | 211    | 2       | `pcmCallback`                                    | `undefined pcmCallback(void)`                                                                           |
| ❌   | `0x005a4e70` | 175    | 2       | `ScriptTemplateMap::Construct`                   | `ScriptTemplateMap * ScriptTemplateMap::Construct(void)`                                                |
| ❌   | `0x005cb2b8` | 23     | 2       | `readBufCreate`                                  | `undefined readBufCreate(void)`                                                                         |
| ❌   | `0x005cb2d0` | 7      | 2       | `readBufDelete`                                  | `undefined readBufDelete(void)`                                                                         |
| ❌   | `0x005cb2d8` | 47     | 2       | `readBufBeginPut`                                | `undefined readBufBeginPut(void)`                                                                       |
| ❌   | `0x005cb308` | 67     | 2       | `readBufEndPut`                                  | `undefined readBufEndPut(void)`                                                                         |
| ❌   | `0x005cb350` | 71     | 2       | `readBufBeginGet`                                | `undefined readBufBeginGet(void)`                                                                       |
| ❌   | `0x005cb398` | 35     | 2       | `readBufEndGet`                                  | `undefined readBufEndGet(void)`                                                                         |
| ❌   | `0x005d2860` | 599    | 3       | `clearGsMem`                                     | `undefined clearGsMem(void)`                                                                            |
| ❌   | `0x005d2ab8` | 895    | 3       | `setImageTag`                                    | `undefined setImageTag(void)`                                                                           |
| ❌   | `0x005d2e38` | 459    | 2       | `handler_endimage`                               | `undefined handler_endimage(void)`                                                                      |
| ❌   | `0x005d3008` | 71     | 2       | `startDisplay`                                   | `undefined startDisplay(void)`                                                                          |
| ❌   | `0x005d3050` | 19     | 2       | `endDisplay`                                     | `undefined endDisplay(void)`                                                                            |
| ❌   | `0x005d3068` | 71     | 2       | `vblankHandler`                                  | `undefined vblankHandler(void)`                                                                         |
| ❌   | `0x005d3a88` | 59     | 2       | `isceSifSendCmd`                                 | `undefined isceSifSendCmd(void)`                                                                        |
| ❌   | `0x005d3fa0` | 71     | 2       | `voBufCreate`                                    | `undefined voBufCreate(void)`                                                                           |
| ❌   | `0x005d3fe8` | 15     | 2       | `voBufReset`                                     | `undefined voBufReset(void)`                                                                            |
| ❌   | `0x005d3ff8` | 19     | 2       | `voBufIsFull`                                    | `undefined voBufIsFull(void)`                                                                           |
| ❌   | `0x005d4010` | 119    | 2       | `voBufIncCount`                                  | `undefined voBufIncCount(void)`                                                                         |
| ❌   | `0x005d4088` | 51     | 2       | `voBufGetData`                                   | `undefined voBufGetData(void)`                                                                          |
| ❌   | `0x005d40c0` | 7      | 2       | `voBufDelete`                                    | `undefined voBufDelete(void)`                                                                           |
| ❌   | `0x005d40d8` | 83     | 2       | `voBufGetTag`                                    | `undefined voBufGetTag(void)`                                                                           |
| ❌   | `0x005d4130` | 31     | 2       | `voBufDecCount`                                  | `undefined voBufDecCount(void)`                                                                         |
| ❌   | `0x005e29e0` | 39     | 6       | `getenv`                                         | `undefined getenv(void)`                                                                                |
| ❌   | `0x005e4510` | 71     | 39      | `SpinDisableInterrupts`                          | `bool SpinDisableInterrupts(void)`                                                                      |
| ❌   | `0x005e4558` | 23     | 44      | `ReenableInterrupts`                             | `bool ReenableInterrupts(void)`                                                                         |
| ❌   | `0x005e4600` | 147    | 3       | `MSInPutBytes`                                   | `int MSInPutBytes(sceCslCtx * pCtx, uint nPort, uchar * pBytes, int nCount)`                            |
| ❌   | `0x005e5a88` | 31     | 4       | `Unit005e5a88::StaticCtor`                       | `undefined Unit005e5a88::StaticCtor(void)`                                                              |
| ❌   | `0x005e5aa8` | 31     | 4       | `Unit005e5aa8::StaticDtor`                       | `undefined Unit005e5aa8::StaticDtor(void)`                                                              |
| ❌   | `0x005e84e0` | 115    | 4       | `_sceVu0ecossin`                                 | `undefined _sceVu0ecossin(void)`                                                                        |
| ❌   | `0x005f1f90` | 31     | 1       | `toupper`                                        | `int toupper(int c)`                                                                                    |
| ❌   | `0x005fa8c0` | 55     | 3       | `PutSioByte`                                     | `undefined PutSioByte(void)`                                                                            |
| ❌   | `0x005fa8f8` | 175    | 1       | `PutSioLineBufferedChar`                         | `undefined PutSioLineBufferedChar(void)`                                                                |
| ❌   | `0x005fa9a8` | 51     | 18      | `PutSioCharCrlf`                                 | `undefined PutSioCharCrlf(void)`                                                                        |
| ❌   | `0x005fa9e0` | 143    | 1       | `ConvertDoubleToScaledInt`                       | `undefined ConvertDoubleToScaledInt(void)`                                                              |
| ❌   | `0x005faa70` | 359    | 1       | `PrintFloatInScientific`                         | `undefined PrintFloatInScientific(void)`                                                                |
| ❌   | `0x005fabd8` | 1479   | 2       | `VPrintfToSioConsole`                            | `undefined VPrintfToSioConsole(void)`                                                                   |
| ❌   | `0x005fb1a0` | 55     | 8       | `PrintfToSioRaw`                                 | `undefined PrintfToSioRaw(void)`                                                                        |
| ❌   | `0x005fb1d8` | 95     | 30      | `PrintfToSioLineBuffered`                        | `undefined PrintfToSioLineBuffered(void)`                                                               |
| ❌   | `0x0060d920` | 231    | 2       | `istdiostream::Slot1`                            | `undefined istdiostream::Slot1(void)`                                                                   |
| ❌   | `0x006134e8` | 243    | 4       | `viBufBeginPut`                                  | `undefined viBufBeginPut(void)`                                                                         |
| ❌   | `0x006135e0` | 83     | 3       | `viBufEndPut`                                    | `undefined viBufEndPut(void)`                                                                           |
| ❌   | `0x00613638` | 271    | 2       | `viBufPutTs`                                     | `undefined viBufPutTs(void)`                                                                            |
| ❌   | `0x00613748` | 75     | 3       | `viBufCount`                                     | `undefined viBufCount(void)`                                                                            |
| ❌   | `0x00620b70` | 287    | 3       | `_findenv_r`                                     | `undefined _findenv_r(void)`                                                                            |

### Measurement history

| Commit    | Share implemented | Main change                                                            |
| --------- | ----------------- | ---------------------------------------------------------------------- |
| `b915bb7` | 88.24%            | Script command layer plus input, powerup, ghost, loop, and track units |
| `ff765e8` | 88.01%            | Script command layer plus test-map builders, spew, test, and clock     |
| `bb0b785` | 87.91%            | Script command layer plus spew and test commands                       |
| `451cec1` | 87.91%            | Script command layer: scene, cheat, toggle, tunnel, HUD, and hx units  |
| `064615f` | 87.50%            | Faithfulness review against the disassembly, four functions found      |
| `5e32890` | 87.52%            | Every declared routine gained a body or a classification               |
| `a40a970` | 85.21%            | Markers added to bodies after a disassembly comparison                 |
| `d7e688b` | 73.39%            | libvu0 and ezmpeg identified as SDK code                               |
| `c35fd29` | 64.19%            | Front end, tunnel, and message container instantiations identified     |
| `7b43c85` | 48.36%            | Front end screens, player, play map, and renderer bodies               |
| `c715051` | 44.79%            | Front end container instantiations identified                          |
| `b7bc378` | 37.58%            | Template library, interpreter, and runtime identification              |
| `72f44cb` | 31.68%            | Template library identification                                        |
| `27cc069` | 27.78%            | Library routines identified by normalised body matches                 |

Since `5e32890`, cross-reviews that traced every argument to its producer and placed every
destructor corrected bodies in every reviewed subsystem (a remix deleted by the wrong key, a
feedback sprite drawn sixteen times too deep, a heap split that did not link its free node, signed
and unsigned comparisons, message lifetimes, and loops that reload a vector's end). Three bodies
moved into headers as inline definitions where another unit expands them. That lowered the counted
bodies by three.

### Known gaps in the measurement

These routines count as remaining although the tree handles them by rule:

- An implicit destructor, copy constructor, or assignment of a project class, labelled
  `<Class>__Destruct`, `__ConstructCopy`, or `__AssignImplicit`. The compiler generates it.
- The ezmpeg sample units (`disp.c`, `vobuf.c`, `readbuf.c`, `strfile.c`, `audiodec.c`,
  `videodec.c`, and `vibuf.c`), linked as shipped.
- The interrupt-context SDK entry points labelled `isce…`. The `sce` pattern does not match them.
- An inline member defined in a header with its `// 0x...` marker (for example
  `Cam::ProjectToUnit`), and a function template instance whose body is the template in a header.
  The body count reads `src` only.
- A definition whose return type clang-format places on a separate line (`CheckPalEqual`,
  `MetJukeboxEditPlaylistScreenLowerLeft::New`). A marker counts only when the next line with code
  identifies the function.
- Static initialiser stubs (`__StaticCtor`, `__StaticDtor`, `__GlobalCtors`) and the exit handlers
  of function-local statics (`AtExitDestroy…`, `__StaticDestroy`).
- The `hx.*` script bindings (the `HxScript__X` bodies and their `HxScript__XEntry` wrappers) and the
  PyCXX wrappers. The interpreter and its C++ binding are not yet in the tree.
- Library routines labelled with their upstream names rather than a family prefix (`getenv`,
  `_findenv_r`, `toupper`, the SIO printf engine, `_sceVu0ecossin`, the `libio` stream slots, and
  the `type_info` and `exception` members).
- Recorded exceptions without a separate body: the `TunnelEvent::DrawFiltered` copy, three
  unreferenced return-zero stubs, the unreferenced send copies in the `Delayer`,
  `MidiDisabler`, and `Renderer` units, and the `Print()` at `0x003d67f0` of the unused class the
  `Q23Mid3MBT` descriptor belongs to.

The body count, `.wiswa-ci/freq/body_share.py`, intersects the body markers under `src` with the
function list after the default exclusions. Its set is 141 routines smaller than the audit's
reconstructable figure (6,781 against 6,922). The implemented share divides by the audit's figure
and therefore understates by at most 141 routines.

Do not measure the implemented share by grepping for address literals. An implementation file
mentions addresses in commentary as well as at body markers.

### Breakdown of exclusions

| Category                       | Count | Basis                                                            |
| ------------------------------ | ----- | ---------------------------------------------------------------- |
| Compiler-generated             | 899   | Type functions, their unfolded per-unit copies, static-init glue |
| Vendored upstream              | 1,978 | CPython 2.0, identified by diagnostic literal                    |
| Per-translation-unit duplicate | 1,950 | Bodies proven byte-identical to another routine of the image     |
| Template library               | 2,877 | Container instantiations                                         |
| Platform SDK                   | 484   | `sce` entry points and kernel syscalls                           |
| C++ runtime                    | 214   | Exception, cast, and unwinding support                           |
| C runtime                      | 323   | String and memory routines, and the floating-point library       |

Exclusions are keyed on the title a function has. A routine is identified before it is excluded,
and the reconstructable figure falls as identification proceeds. The rules match prefixes applied
deliberately: `Stl` and `std_` for the template library, `Cxx` and the iostream names for the C++
runtime, `Lib` with a following c, k, or m for the vendored C library, kernel glue, and floating
point, `sce` for the platform SDK, `Py` and `Python__` for the interpreter, and `Gzip` and `Netflow`
for two further vendored packages. The stdio routines are listed individually. A prefix there would
match project routines.

The platform SDK row still counts the Sony SDK routines that ps2sdk does not provide. Those are owed
code (see [Platform division](#platform-division)) and move out of the exclusion as they are
separated.

## Verification

Every figure below comes from a command. CI compiles every buildable source with the Emotion Engine
cross compiler in the ps2dev container and archives one static library per subsystem. The libraries
are not linked yet. CI reports two recorded warnings. `remixindex.h` reports five fields that
`RemixIndex::ReadFromStream()` copies before they are written, as the binary does. `mem.cpp` lacks
the sized `operator delete` forms, and the original compiler predates them.

| Check                                 | Status       |
| ------------------------------------- | ------------ |
| Headers compiling standalone          | 706/706      |
| Sources compiling                     | 627/627      |
| Address annotations with no function  | 0            |
| Lines over 100 characters             | 0            |
| `clang-format` differences            | 0            |
| `cspell`                              | 0 issues     |
| Declared virtuals resolving to a base | 0 mismatches |

Sources are measured by `.wiswa-ci/freq/syntax_check.sh` and headers by
`.wiswa-ci/freq/header_check.sh`. Each header compiles in a translation unit that includes only that
header. Both scripts compile with `-Wall -Wextra` against ps2sdk and the stand-ins for Sony SDK
headers, and both report their skip count. The 26 headers and 20 sources that include the
interpreter's `Python.h` are skipped by the scripts and checked with the line below, run from
`freq-src`. The first two rows count them.

```shell
g++ -fsyntax-only -std=c++17 -Wall -Wextra -D_EE -DHAVE_LIMITS_H -DSIZEOF_LONG=8 -I include -I compat -isystem src/python/PC -isystem ../.wiswa-ci/freq/Python-2.0/Include -isystem ../ps2sdk/common/include -isystem ../ps2sdk/ee/kernel/include -isystem ../ps2sdk/ee/rpc/cdvd/include -isystem ../ps2sdk/ee/rpc/sdr/include -isystem ../ps2sdk/ee/rpc/memorycard/include -isystem ../ps2sdk/ee/rpc/multitap/include <file>
```

`-DSIZEOF_LONG=8` is the original target's width. The interpreter's `PyInt_AsLong()` at
`0x00581c00` reads an integer object's `long` with an eight-byte `ld`. A current PlayStation 2
toolchain has a four-byte `long`.

The override check is `.wiswa-ci/freq/check_overrides.py` over `freq-src/include`. It reports a
declared virtual whose title matches a base virtual only in letter case, and one that matches exactly
with a different parameter count. Neither produces a compiler diagnostic. The class stops overriding
and its table grows past the entry count the image shows. Pass the directory rather than a file
list. Without the base headers in scope every virtual appears new.

The annotation scanner recognises the bare `// 0x...` comment above a file-local routine as well as
the Doxygen tag. The comment has to sit on a separate line with a definition after it.

## Subsystems

| Area                          | Notes                                                                                                                                                                                                |
| ----------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Entry point                   | `main` and the loading screen                                                                                                                                                                        |
| Asynchronous file layer       | Submission, the drive callback, the request and job records                                                                                                                                          |
| Animation base                | `Rnd::Animatable` with all five nested filters                                                                                                                                                       |
| Collision base                | `Rnd::Collideable` with its hit and sink types                                                                                                                                                       |
| Message and packet family     | 73 concrete classes, 22 of them packets, over `Message`, `Packet`, `CmdMsg`, `MuseMsg`, and seven routing intermediates                                                                              |
| Material, PlayStation 2       | `Rnd::PsMat`, including the blend mode table                                                                                                                                                         |
| Streams                       | File, buffer, memory, and tool streams, and the nine-class byte stream family over the input and output interfaces                                                                                   |
| Mesh, PlayStation 2           | `Sync` and all four draw paths, software and VU1                                                                                                                                                     |
| GIF packet buffer             | Reservation, tag closing, and the scratchpad double buffer                                                                                                                                           |
| Scheduler command base        | `Sch::Command`, `Sch::TimedCommand`, and `Sch::Tick`                                                                                                                                                 |
| Transform and animation base  | `Rnd::TransAnim` with its keyframe channels                                                                                                                                                          |
| View, camera, and environment | `Rnd::Blur`, `Rnd::View`, `PsCam`, and `PsEnviron`, with camera and environment serialisation                                                                                                        |
| Mesh animation and instancing | `Rnd::MeshAnim` with its three keyframe channels, `Rnd::MultiMesh`, and `Rnd::PsMultiMesh`                                                                                                           |
| Art library                   | Every canvas class, the polygon fills, the stretch and clip routines, `APalette`, `ARleReader`, and the BMP, TGA, and GIF readers, apart from the two owed routines above                            |
| Texture, PlayStation 2        | `Rnd::PsTex`, including surface restore, the upload and bind path, and render-target binding                                                                                                         |
| Particles                     | `Rnd::ParticleSys`, including the simulation, the text dump, and revisions 0 to 6 of its file format                                                                                                 |
| Cutscene player               | The game's C unit around Sony's MPEG sample                                                                                                                                                          |
| Debug console                 | The development console bring-up                                                                                                                                                                     |
| Exception runtime             | Identified rather than reconstructed. The scheme is DWARF                                                                                                                                            |
| Graphics device               | Every `GfxDevice` routine, with the VU1 setup, lighting, and clipping routines of the draw path. The vertical-blank handler's body is MIPS assembly                                                  |
| Tunnel                        | `Rnd::Tunnel`, `Rnd::Generator`, the duration-gem trails, `AppTunnel`, and the tunnel effect classes                                                                                                 |
| Gameplay display              | `Renderer`, `Overlay`, the HUD panel and per-track display, `TnlArena`, and the screen animations                                                                                                    |
| Gameplay world                | `GrooveWorld`, `Phrase`, `PhraseDatabase`, `PhraseMgr`, `TrackData`, `Catcher`, `AutoRiffer`, `NotePlayer`, the power-bar managers, the R250 generator, and the pitcher classes                      |
| Messages and packets          | Every factory, printer, serializer, and stack constructor of the packets and messages, with the Standard MIDI File reader                                                                            |
| Sound                         | `Synth` and `Ps2HardSynth` with the interface mapped slot by slot, and the bank path of `midi_main`. The module drives the hardware through libsdr. Thirteen interface slot titles are unrecoverable |
| Script commands               | The scene, cheat, toggle, tunnel, HUD, record, test-map, input, powerup, ghost, loop, track-control, and `hx` command implementations with their interpreter entry points, over the `Py::` binding   |

### Parked questions

Two types compete for the class `Mid::MBT`. The image's `Q23Mid3MBT` descriptor belongs to an unused
0x10-byte polymorphic class (measure, beat, tick, and a vptr at +0xc, with its vtable at
0x008110e8, constructor at 0x003d6798, and `Print()` at 0x003d67f0). The four-byte position word
the messages use emits no RTTI, and its true identifier is not in the image. The tree retains
`Mid::MBT` for the four-byte word by convention until evidence of its real identifier appears.

## Duplicated routines

An address count is not a function count in this image. The toolchain emits an inline function into
every translation unit that needs it, and the linker does not fold the copies. One class has 45
identical copies of its type function.

A group of byte-identical routines is one routine emitted many times and needs at most one source
definition. A group whose members share an opcode shape but differ in their immediates is many
distinct routines (two classes' destructors referencing different vtables, for example). Byte
equality against the image is the only test that separates the two kinds.

A `CopyN` suffix records that byte test. A group's original, the copy callers or the unit's unwind
record identify, takes the plain title. A trivial destructor stores its class's vptr and does
nothing else, and every class sharing one base compiles an identical one. Occupying a vtable slot
therefore marks a distinct method, and the duplicate pass refuses any address in a vtable slot.

The vendored interpreter publishes a type object per type, whose slots address that type's static
functions in a fixed order, and a method table per module and per type. The upstream tree supplies
the same orders by name, and joining the two recovers exact upstream names. The embedded interpreter
differs from the 2.0 release. Its dictionary type fills a rich-comparison slot the release does not
fill and does not fill two the release fills.

### Identification levers

Three patterns identify a routine from its contents. A class's allocation operator is a short body
that calls the tagged allocator with its class name as a string. A type_info accessor passes its
class's length-prefixed mangled name to the descriptor constructor. Slot zero of a class's vtable
is that accessor, and walking the table to its zero terminator enumerates the class's virtuals.
Every type_info accessor in the image includes its class's name.

A pass that reports no findings looks exactly like a subsystem with no remaining work. A negative
result needs a control, and the control has to exercise the part that can be wrong.

The node colour of the template library's red-black tree is an `int` rather than a byte. The absence
of byte operations is not evidence against a tree.

## Conventions

An `@ghidraAddress 0x...` tag on a declaration ties it to the routine it was reconstructed from. The
address is relative to the image base.

A gap is marked rather than filled. A member whose purpose is undetermined has a placeholder
identifier and an offset comment, an inferred identifier is documented as inferred, and a reserved
run records an unrecovered span of a structure.

### Compiler-generated code is not written

A virtual function table pointer, a virtual base pointer, a type function, the per-unit
static-initialisation glue, and an implicit copy or assignment body are emitted by the compiler. The
tree does not declare or define any of them. A layout comment records where the compiler placed a
pointer.

### Access specifiers are inferred

A compiled image does not record access control, and every specifier in this tree is an inference
from how the code uses a member. A data member of a class with behaviour is private by default. It
becomes protected when a derived class uses it, and public when code outside the hierarchy does.

### STL container layout

The template library is the SGI implementation that g++ 2.9x shipped. A `std::list` is one
four-byte pointer to a single self-linked dummy node. A four-byte element places the value at
`+0x08` in a 16-byte node, and a 16-byte-aligned element places it at `+0x10` in a 0x50-byte node.
Read the node size and the element size from the allocation.

A container instantiation is library code. The tree writes the operator or the algorithm call the
original wrote, not a reconstructed body.

### Integer widths

The original toolchain's `long` is eight bytes wide and its pointers are four. The tree writes a
value the image stores in eight bytes as `long long`. It retains `long` only where it mirrors an
interface declared with `long` (the interpreter and its C++ binding, the `IBStream` and `OBStream`
overloads, and a `%ld` format argument).

### Platform division

A class whose title begins `Ps` is the PlayStation 2 implementation of the portable class above it.
Reconstruction covers the portable and the PlayStation 2 sides. A stub for another port is marked
as a stub and does not invent a second platform's behaviour.

The original was built against Sony's official SDK. Every SDK call is checked against its
declaration in ps2sdk, and a forwarding shim resolves header differences. Routines ps2sdk provides
are not reconstructed. Routines of the official SDK that ps2sdk lacks are owed code and are
reconstructed like game code.

## Methodology notes

A routine that prints its identifier in a diagnostic is authoritative about it. That recovered
`RestoreSurfaces` after this tree had called it `OnAllMipsLoaded`.

A value in the return register is a return only when every exit agrees on it and the value is not an
address.

A call site that materialises an argument proves the prototype declares it. Only the callee proves
whether the body reads it.

A type function builds its bases before itself. The owner is the class whose descriptor the routine
tests at entry, not the first constructor call inside it.

A routine testing its second argument against `0xffff` and branching on its first is the
per-translation-unit static-initialisation glue of this compiler. It is not source.

A trailing all-zero vtable entry is a terminator rather than a null slot.

Two byte-identical short bodies at different addresses are usually two distinct trivial overrides
of one pure virtual. One address appearing in several vtables is a shared base body.

A reported success from the disassembler bridge does not prove a write persisted. Every write is
re-read, and a batch is applied in a loop until the read-back agrees.

A forward declaration belongs inside the class's namespace. A global `class Mat;` for `Rnd::Mat`
declares a second unrelated type, and the failure appears in a distant implementation file.

A detector that pattern-matches source blanks the comments first. An unchanged count after adding an
input is a failure report rather than a stable baseline.

An address built by a `lui` and `addiu` pair is evaluated rather than read off. The low half is
signed, and `addiu v0,v0,0x88c0` subtracts 0x7740.

Where another unit expands a routine without calling it, the routine was inline in a header. The
compiler cannot inline a body it does not see.
