#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "GMSJ01",  # 0
    "GMSP01",  # 1
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-1"
config.compilers_tag = "20250520"
config.dtk_tag = "v1.3.0"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.0"
config.wibo_tag = "0.6.11"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    "-I include/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common",
    "-I include/PowerPC_EABI_Support/Msl/MSL_C++/MSL_Common",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
config.ldflags = [
    "-fp hardware",
    "-nodefaults",
]
if args.debug:
    config.ldflags.append("-g")  # Or -gdwarf-2 for Wii linkers
if args.map:
    config.ldflags.append("-mapunused")
    # config.ldflags.append("-listclosure") # For Wii linkers

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = []

# Optional numeric ID for decomp.me preset
# Can be overridden in libraries or objects
config.scratch_preset_id = 61  # Super Mario Sunshine

# Base flags, common to most GC/Wii games.
# Generally leave untouched, with overrides added below.
cflags_base_base = [
    "-nodefaults",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    '-pragma "cats off"',
    '-pragma "warn_notinlined off"',
    "-maxerrors 1",
    "-nosyspath",
    "-RTTI off",
    "-str reuse",
    "-multibyte",  # For Wii compilers, replace with `-enc SJIS`
    "-cwd source",
    "-i include",
    "-i include/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common",
    "-i include/PowerPC_EABI_Support/Msl/MSL_C++/MSL_Common",
    f"-i build/{config.version}/include",
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]

cflags_base = [
    *cflags_base_base,
    "-proc gekko",
    "-DGEKKO",
]

# Debug flags
if args.debug:
    # Or -sym dwarf-2 for Wii compilers
    cflags_base.extend(["-sym on", "-DDEBUG=1"])
else:
    cflags_base.append("-DNDEBUG=1")

# Metrowerks library flags
cflags_runtime = [
    *cflags_base,
    "-O4,p",
    "-inline auto",
    "-fp_contract on",
    "-str reuse,pool,readonly",
    "-inline deferred,auto",
]

cflags_jsystem = [
    *cflags_base,
    "-O4,p",
    "-opt all,nostrength",
    "-inline auto",
    "-str reuse,readonly",
    "-lang=c++",
    "-fp hard",
    "-fp_contract on",
    "-char signed",
    "-rostr",
    "-common on"
]

cflags_jsystem_dsp = [
    *cflags_base_base,
    "-lang=c++",
    "-proc 750",
    "-O4",
    "-str readonly",
    "-func_align 32",
]

cflags_game = [
    *cflags_base,
    "-O4,p",
    "-inline auto",
    "-fp_contract on",
    "-str reuse,readonly",
]

cflags_system = [
    *cflags_game,
    "-inline auto",
    "-opt all,nostrength",
]

cflags_dolphin = [
    *cflags_base,
    "-O4,p",
    "-inline auto",
    "-fp_contract off", # NOTE: this is definitely off according to mtx.c
    # TODO: should these be different?
]

config.linker_version = "GC/1.2.5"


# Some parts of the SDK were compiled with the 1.2.5n patch, some weren't
def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5n",
        "cflags": cflags_dolphin,
        "progress_category": "sdk",
        "objects": objects,
    }

def DolphinLibUnpatched(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": "GC/1.2.5",
        "cflags": cflags_dolphin,
        "progress_category": "sdk",
        "objects": objects,
    }


Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


# Object is only matching for specific versions
def MatchingFor(*versions):
    return config.version in versions


config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "main",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_game,
        "progress_category": "game",
        "objects": [
            Object(Matching, "main.cpp"),
        ],
    },
    {
        "lib": "JSystem",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_jsystem,
        "progress_category": "jsystem",
        "objects": [
            # JSupport
            Object(Matching, "JSystem/JSupport/JSUOutputStream.cpp"),
            Object(Equivalent, "JSystem/JSupport/JSUInputStream.cpp"),
            Object(Matching, "JSystem/JSupport/JSUList.cpp"),
            Object(Matching, "JSystem/JSupport/JSUMemoryStream.cpp"),
            Object(Matching, "JSystem/JSupport/JSUFileStream.cpp"),

            # JGadget
            Object(Matching, "JSystem/JGadget/std-list.cpp"),
            Object(Equivalent, "JSystem/JGadget/std-vector.cpp"),
            Object(Matching, "JSystem/JGadget/linklist.cpp"),
            Object(Matching, "JSystem/JGadget/singlelinklist.cpp"),

            # JKernel
            Object(Equivalent, "JSystem/JKernel/JKRArchivePri.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRAramArchive.cpp"),
            Object(Matching, "JSystem/JKernel/JKRAramHeap.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRAram.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRArchivePub.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRCompArchive.cpp"),
            Object(Matching, "JSystem/JKernel/JKRDisposer.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRDvdArchive.cpp"),
            Object(Matching, "JSystem/JKernel/JKRDvdFile.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRExpHeap.cpp"),
            Object(Matching, "JSystem/JKernel/JKRFileCache.cpp"),
            Object(Matching, "JSystem/JKernel/JKRFileFinder.cpp"),
            Object(Matching, "JSystem/JKernel/JKRFileLoader.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRHeap.cpp"),
            Object(Matching, "JSystem/JKernel/JKRMemArchive.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRSolidHeap.cpp"),
            Object(Matching, "JSystem/JKernel/JKRStdHeap.cpp"),
            Object(Matching, "JSystem/JKernel/JKRThread.cpp"),
            Object(Matching, "JSystem/JKernel/JKRDvdRipper.cpp"),
            Object(Equivalent, "JSystem/JKernel/JKRDvdAramRipper.cpp"),
            Object(Matching, "JSystem/JKernel/JKRDecomp.cpp"),
            Object(Matching, "JSystem/JKernel/JKRAramBlock.cpp"),
            Object(Matching, "JSystem/JKernel/JKRAramPiece.cpp"),
            Object(Matching, "JSystem/JKernel/JKRAramStream.cpp"),

            # JUtility
            Object(Equivalent, "JSystem/JUtility/JUTException.cpp"),
            Object(Matching, "JSystem/JUtility/JUTDirectPrint.cpp"),
            Object(Matching, "JSystem/JUtility/JUTDbPrint.cpp"),
            Object(Matching, "JSystem/JUtility/JUTFont.cpp"),
            Object(Equivalent, "JSystem/JUtility/JUTGamePad.cpp"),
            Object(Equivalent, "JSystem/JUtility/JUTNameTab.cpp"),
            Object(Matching, "JSystem/JUtility/JUTPalette.cpp"),
            Object(Matching, "JSystem/JUtility/JUTRect.cpp"),
            Object(Matching, "JSystem/JUtility/JUTResource.cpp"),
            Object(Matching, "JSystem/JUtility/JUTTexture.cpp"),
            Object(Matching, "JSystem/JUtility/JUTAssert.cpp"),
            Object(Matching, "JSystem/JUtility/JUTVideo.cpp"),
            Object(Matching, "JSystem/JUtility/JUTResFont.cpp"),
            Object(Matching, "JSystem/JUtility/JUTRomFont.cpp"),
            Object(Equivalent, "JSystem/JUtility/JUTConsole.cpp"),
            Object(Equivalent, "JSystem/JUtility/JUTDirectFile.cpp"),

            # JDrama
            Object(Equivalent, "JSystem/JDrama/JDRActor.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRCamera.cpp"),
            Object(Matching, "JSystem/JDrama/JDRCharacter.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRDirector.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRDisplay.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRDrawBufObj.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRDStage.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRDStageGroup.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDREfbCtrl.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDREfbSetting.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRFrmGXSet.cpp"),
            Object(Matching, "JSystem/JDrama/JDRGraphics.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRLighting.cpp"),
            Object(Matching, "JSystem/JDrama/JDRNameRef.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRNameRefGen.cpp"),
            Object(Matching, "JSystem/JDrama/JDRPlacement.cpp"),
            Object(Matching, "JSystem/JDrama/JDRRenderMode.cpp"),
            Object(Matching, "JSystem/JDrama/JDRResolution.cpp"),
            Object(Matching, "JSystem/JDrama/JDRScreen.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRSmJ3DAct.cpp"),
            Object(Equivalent, "JSystem/JDrama/JDRSmJ3DScn.cpp"),
            Object(Matching, "JSystem/JDrama/JDRVideo.cpp"),
            Object(Matching, "JSystem/JDrama/JDRViewConnecter.cpp"),
            Object(Matching, "JSystem/JDrama/JDRViewObj.cpp"),
            Object(Matching, "JSystem/JDrama/JDRViewport.cpp"),

            # JAudio
            # JADebug
            Object(Matching, "JSystem/JAudio/JADebug/JADHioNode.cpp"),
            # JALibrary
            Object(Equivalent, "JSystem/JAudio/JALibrary/JALCalc.cpp"),
            Object(Equivalent, "JSystem/JAudio/JALibrary/JALModSe.cpp"),
            # JAInterface
            Object(Matching, "JSystem/JAudio/JAInterface/JAIAsnData.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIAnimation.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIBasic.cpp"),
            Object(Matching, "JSystem/JAudio/JAInterface/JAIConst.cpp"),
            Object(Matching, "JSystem/JAudio/JAInterface/JAIDebug.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIData.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIEntry.cpp"),
            Object(Matching, "JSystem/JAudio/JAInterface/JAIEntrySe.cpp"),
            Object(Matching, "JSystem/JAudio/JAInterface/JAIEntrySequence.cpp"),
            Object(Matching, "JSystem/JAudio/JAInterface/JAIEntryStream.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIGFrameSe.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIGFrameSequence.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIGFrameStream.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAIGlobalParameter.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAISound.cpp"),
            Object(Equivalent, "JSystem/JAudio/JAInterface/JAISystemInterface.cpp"),
            # JASystem
            Object(Matching, "JSystem/JAudio/JASystem/JASBank.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASBankMgr.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASBasicBank.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASBasicInst.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASBasicWaveBank.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASBNKParser.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASDrumSet.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASInstEffect.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASInstRand.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASInstSense.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASSimpleWaveBank.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASWaveArcLoader.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASWaveBank.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASWaveBankMgr.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASWSParser.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASAudioThread.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASChAllocQueue.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASChannel.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASDriverTables.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASChannelMgr.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASChGlobal.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASDriverIF.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASDSPBuf.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASDSPChannel.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASDSPInterface.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASOscillator.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASAiCtrl.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASCalc.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASCallback.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASCmdStack.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASDvdThread.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASHardStream.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASHeapCtrl.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASRate.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASKernelDebug.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASProbe.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASSystemHeap.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASVload.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASNoteMgr.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASOuterParam.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASPlayer_impl.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASRegisterParam.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASSeqCtrl.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASTrack.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASTrackInterrupt.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASTrackMgr.cpp"),
            Object(Matching, "JSystem/JAudio/JASystem/JASTrackPort.cpp"),
            Object(Equivalent, "JSystem/JAudio/JASystem/JASSeqParser.cpp"),

            # J2D
            Object(Matching, "JSystem/J2D/J2DPane.cpp"),
            Object(Equivalent, "JSystem/J2D/J2DPicture.cpp"),
            Object(Matching, "JSystem/J2D/J2DPrint.cpp"),
            Object(Matching, "JSystem/J2D/J2DScreen.cpp"),
            Object(Matching, "JSystem/J2D/J2DTextBox.cpp"),
            Object(Equivalent, "JSystem/J2D/J2DWindow.cpp"),
            Object(Matching, "JSystem/J2D/J2DGrafContext.cpp"),
            Object(Matching, "JSystem/J2D/J2DOrthoGraph.cpp"),

            # J3D
            # J3DGraphBase
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DTransform.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DMaterial.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DShape.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DSys.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DVertex.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DPacket.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DTevs.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphBase/J3DDrawBuffer.cpp"),
            # J3DGraphAnimator
            Object(Equivalent, "JSystem/J3D/J3DGraphAnimator/J3DCluster.cpp"),
            Object(Equivalent, "JSystem/J3D/J3DGraphAnimator/J3DJoint.cpp"),
            Object(Equivalent, "JSystem/J3D/J3DGraphAnimator/J3DModel.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphAnimator/J3DNode.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphAnimator/J3DAnimation.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphAnimator/J3DMaterialAttach.cpp"),
            # J3DGraphLoader
            Object(Matching, "JSystem/J3D/J3DGraphLoader/J3DClusterLoader.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphLoader/J3DJointFactory.cpp"),
            Object(Equivalent, "JSystem/J3D/J3DGraphLoader/J3DMaterialFactory.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphLoader/J3DModelLoader.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphLoader/J3DShapeFactory.cpp"),
            Object(Matching, "JSystem/J3D/J3DGraphLoader/J3DAnmLoader.cpp"),
            Object(Equivalent, "JSystem/J3D/J3DGraphLoader/J3DMaterialFactory_v21.cpp"),

            # JParticle
            Object(Equivalent, "JSystem/JParticle/JPAEmitter.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPAEmitterManager.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPAField.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPAMath.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPAParticle.cpp"),
            Object(Matching, "JSystem/JParticle/JPADrawSetupTev.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPADraw.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPADrawVisitor.cpp"),
            Object(Matching, "JSystem/JParticle/JPAExTexShape.cpp"),
            Object(Matching, "JSystem/JParticle/JPADataBlock.cpp"),
            Object(Matching, "JSystem/JParticle/JPAEmitterLoader.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPABaseShape.cpp"),
            Object(Matching, "JSystem/JParticle/JPAExtraShape.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPAResourceManager.cpp"),
            Object(Matching, "JSystem/JParticle/JPASweepShape.cpp"),
            Object(Equivalent, "JSystem/JParticle/JPATexture.cpp"),

            # JStage
            Object(Matching, "JSystem/JStage/JSGAmbientLight.cpp"),
            Object(Matching, "JSystem/JStage/JSGCamera.cpp"),
            Object(Matching, "JSystem/JStage/JSGActor.cpp"),
            Object(Matching, "JSystem/JStage/JSGLight.cpp"),
            Object(Matching, "JSystem/JStage/JSGObject.cpp"),
            Object(Matching, "JSystem/JStage/JSGSystem.cpp"),

            Object(Matching, "JSystem/JMath.cpp"),
            Object(Matching, "JSystem/JRenderer.cpp"),
            Object(Matching, "JSystem/random.cpp"),
            Object(Matching, "JSystem/dspproc.c", cflags=cflags_jsystem_dsp, mw_version="GC/1.2.5n"),
            Object(Matching, "JSystem/dsptask.c", cflags=cflags_jsystem_dsp, mw_version="GC/1.2.5n"),
            Object(Matching, "JSystem/osdsp.c", cflags=cflags_jsystem_dsp, mw_version="GC/1.2.5n"),
            Object(Matching, "JSystem/osdsp_task.c", cflags=cflags_jsystem_dsp, mw_version="GC/1.2.5n"),
        ],
    },
    {
        "lib": "Runtime.PPCEABI.H",
        "progress_category": "sdk",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_runtime,
        "objects": [
            Object(Matching, "PowerPC_EABI_Support/Runtime/__mem.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/__va_arg.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/global_destructor_chain.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/NMWException.cp"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/ptmf.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/ExceptionPPC.cp"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/runtime.c"),
            Object(Matching, "PowerPC_EABI_Support/Runtime/__init_cpp_exceptions.cpp"),
        ],
    },
    {
        "lib": "MSL_C.PPCEABI.bare.H",
        "progress_category": "sdk",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_runtime,
        "objects": [
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/ansi_files.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/abort_exit.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/errno.c"),
            Object(Equivalent, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/uart_console_io.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/buffer_io.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/PPC_EABI/critical_regions.ppc_eabi.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/ctype.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/direct_io.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/mbstring.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/mem.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/mem_funcs.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/misc_io.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/printf.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/rand.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/scanf.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/string.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/strtoul.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/float.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/wchar_io.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_asin.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Double_precision/w_atan2.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/hyperbolicsf.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/inverse_trig.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/trigf.c"),
            Object(Matching, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/common_float_tables.c"),
            Object(Equivalent, "PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf.c"),
        ],
    },
    {
        "lib": "TRK_MINNOW_DOLPHIN",
        "mw_version": "GC/1.1p1",
        "cflags": [*cflags_base, "-O4,p", "-pool off", "-str readonly", "-enum min", "-sdatathreshold 0"],
        "progress_category": "sdk",
        "objects": [
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/mainloop.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/nubevent.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/nubinit.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/msg.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/msgbuf.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/serpoll.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Os/dolphin/usr_put.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/dispatch.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/msghndlr.c"),
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/mutex_TRK.c"),
            Object(Equivalent, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Processor/ppc/Generic/flush_cache.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/mem_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Processor/ppc/Generic/targimpl.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/__exception.s"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Processor/ppc/Generic/mpc_7xx_603e.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/main_TRK.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Os/dolphin/dolphin_trk_glue.c"),
            Object(Matching, "TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Os/dolphin/targcont.c"),
        ],
    },
    DolphinLib("base", [
            Object(Matching, "dolphin/base/PPCArch.c"),
        ]),
    DolphinLib("db", [
            Object(Matching, "dolphin/db/db.c"),
        ]),
    DolphinLib("os", [
            Object(Matching, "dolphin/os/OS.c"),
            Object(Matching, "dolphin/os/OSAlarm.c"),
            Object(Matching, "dolphin/os/OSAlloc.c"),
            Object(Matching, "dolphin/os/OSArena.c"),
            Object(Matching, "dolphin/os/OSAudioSystem.c"),
            Object(Matching, "dolphin/os/OSCache.c"),
            Object(Matching, "dolphin/os/OSContext.c"),
            Object(Matching, "dolphin/os/OSError.c"),
            Object(Matching, "dolphin/os/OSFont.c"),
            Object(Matching, "dolphin/os/OSInterrupt.c"),
            Object(Matching, "dolphin/os/OSLink.c"),
            Object(Matching, "dolphin/os/OSMessage.c"),
            Object(Matching, "dolphin/os/OSMemory.c"),
            Object(Matching, "dolphin/os/OSMutex.c"),
            Object(Matching, "dolphin/os/OSReboot.c"),
            Object(Matching, "dolphin/os/OSReset.c"),
            Object(Matching, "dolphin/os/OSResetSW.c"),
            Object(Matching, "dolphin/os/OSRtc.c"),
            Object(Matching, "dolphin/os/OSStopwatch.c"),
            Object(Matching, "dolphin/os/OSSync.c"),
            Object(Matching, "dolphin/os/OSThread.c"),
            Object(Matching, "dolphin/os/OSTime.c"),
            Object(Matching, "dolphin/os/__start.c"),
            Object(Matching, "dolphin/os/__ppc_eabi_init.cpp"),
        ]),
    DolphinLibUnpatched("mtx", [
            Object(Matching, "dolphin/mtx/mtx.c"),
            Object(Matching, "dolphin/mtx/mtxvec.c"),
            Object(Matching, "dolphin/mtx/mtx44.c"),
            Object(Matching, "dolphin/mtx/vec.c"),
        ]),
    DolphinLib("dvd", [
            Object(Matching, "dolphin/dvd/dvdlow.c"),
            Object(Matching, "dolphin/dvd/dvdfs.c"),
            Object(Matching, "dolphin/dvd/dvd.c"),
            Object(Matching, "dolphin/dvd/dvdqueue.c"),
            Object(Matching, "dolphin/dvd/dvderror.c"),
            Object(Matching, "dolphin/dvd/fstload.c"),
        ]),
    DolphinLib("vi", [
            Object(Matching, "dolphin/vi/vi.c"),
        ]),
    DolphinLib("pad", [
            Object(Matching, "dolphin/pad/Padclamp.c"),
            Object(Matching, "dolphin/pad/Pad.c"),
        ]),
    DolphinLib("ai", [
            Object(Matching, "dolphin/ai/ai.c"),
        ]),
    DolphinLib("ar", [
            Object(Matching, "dolphin/ar/ar.c"),
            Object(Matching, "dolphin/ar/arq.c"),
        ]),
    DolphinLib("dsp", [
            Object(Matching, "dolphin/dsp/dsp.c"),
            Object(Matching, "dolphin/dsp/dsp_debug.c"),
            Object(Matching, "dolphin/dsp/dsp_task.c"),
        ]),
    DolphinLib("card", [
            Object(Matching, "dolphin/card/CARDBios.c"),
            Object(Matching, "dolphin/card/CARDUnlock.c"),
            Object(Matching, "dolphin/card/CARDRdwr.c"),
            Object(Matching, "dolphin/card/CARDBlock.c"),
            Object(Matching, "dolphin/card/CARDDir.c"),
            Object(Matching, "dolphin/card/CARDCheck.c"),
            Object(Matching, "dolphin/card/CARDMount.c"),
            Object(Matching, "dolphin/card/CARDFormat.c"),
            Object(Matching, "dolphin/card/CARDOpen.c"),
            Object(Matching, "dolphin/card/CARDCreate.c"),
            Object(Matching, "dolphin/card/CARDRead.c"),
            Object(Matching, "dolphin/card/CARDWrite.c"),
            Object(Matching, "dolphin/card/CARDStat.c"),
            Object(Matching, "dolphin/card/CARDNet.c"),
        ]),
    DolphinLib("gx", [
            Object(Matching, "dolphin/gx/GXInit.c"),
            Object(Matching, "dolphin/gx/GXFifo.c"),
            Object(Matching, "dolphin/gx/GXAttr.c"),
            Object(Matching, "dolphin/gx/GXMisc.c"),
            Object(Matching, "dolphin/gx/GXGeometry.c"),
            Object(Matching, "dolphin/gx/GXFrameBuf.c"),
            Object(Matching, "dolphin/gx/GXLight.c"),
            Object(Matching, "dolphin/gx/GXTexture.c"),
            Object(Matching, "dolphin/gx/GXBump.c"),
            Object(Matching, "dolphin/gx/GXTev.c"),
            Object(Matching, "dolphin/gx/GXPixel.c"),
            Object(Matching, "dolphin/gx/GXDraw.c"),
            Object(Matching, "dolphin/gx/GXStubs.c"),
            Object(Matching, "dolphin/gx/GXDisplayList.c"),
            Object(Matching, "dolphin/gx/GXTransform.c"),
            Object(Matching, "dolphin/gx/GXPerf.c"),
        ]),
    DolphinLib("OdemuExi2", [
            Object(Matching, "OdemuExi2/DebuggerDriver.c", cflags=[*cflags_dolphin, "-inline auto,deferred"]),
        ]),
    DolphinLib("amcstubs", [
            Object(Matching, "dolphin/amcstubs/AmcExi2Stubs.c"),
        ]),
    DolphinLib("odenotstub", [
            Object(Matching, "dolphin/odenotstub/odenotstub.c"),
        ]),
    DolphinLib("gd", [
            Object(Matching, "dolphin/gd/GDBase.c"),
            Object(Matching, "dolphin/gd/GDGeometry.c"),
            Object(Matching, "dolphin/gd/GDLight.c"),
            Object(Matching, "dolphin/gd/GDPixel.c"),
            Object(Matching, "dolphin/gd/GDTev.c"),
            Object(Matching, "dolphin/gd/GDTransform.c"),
        ]),
    DolphinLib("si", [
            Object(Matching, "dolphin/si/SIBios.c"),
            Object(Matching, "dolphin/si/SISamplingRate.c"),
        ]),
    DolphinLib("exi", [
            Object(Matching, "dolphin/exi/EXIBios.c"),
            Object(Matching, "dolphin/exi/EXIUart.c"),
        ]),
    DolphinLibUnpatched("thp", [
            Object(Matching, "dolphin/thp/THPDec.c"),
            Object(Matching, "dolphin/thp/THPAudio.c"),
        ]),
    # Note that this is NOT in fact part of the SDK, as it integrates
    # with jsystem and game code
    {
        "lib": "THPPlayer",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_base, "-O4,p", "-inline auto", "-fp_contract on", "-str reuse,readonly", "-lang=c++", "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "THPPlayer/THPAudioDecode.c"),
            Object(Matching, "THPPlayer/THPDraw.c"),
            Object(Equivalent, "THPPlayer/THPPlayer.c"),
            Object(Matching, "THPPlayer/THPRead.c"),
            Object(Matching, "THPPlayer/THPVideoDecode.c"),
        ],
    },
    {
        "lib": "MarioUtil",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred ", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Matching, "MarioUtil/DLUtil.cpp"),
            Object(Equivalent, "MarioUtil/DrawUtil.cpp"),
            Object(Equivalent, "MarioUtil/LightUtil.cpp"),
            Object(Equivalent, "MarioUtil/MathUtil.cpp"),
            Object(Equivalent, "MarioUtil/MtxUtil.cpp"),
            Object(Equivalent, "MarioUtil/ScreenUtil.cpp"),
            Object(Equivalent, "MarioUtil/ShadowUtil.cpp"),
            Object(Matching, "MarioUtil/gd-reinit-gx.cpp"),
            Object(Equivalent, "MarioUtil/EffectUtil.cpp"),
            Object(Equivalent, "MarioUtil/ModelUtil.cpp"),
            Object(Matching, "MarioUtil/RumbleMgr.cpp"),
            Object(Matching, "MarioUtil/RumbleData.cpp"),
            Object(Matching, "MarioUtil/RumbleType.cpp"),
            Object(Equivalent, "MarioUtil/PacketUtil.cpp"),
            Object(Matching, "MarioUtil/GDUtil.cpp"),
            Object(Matching, "MarioUtil/TexUtil.cpp"),
            Object(Matching, "MarioUtil/MapUtil.cpp"),
            Object(Matching, "MarioUtil/ToolData.cpp"),
        ],
    },
    {
        "lib": "M3DUtil",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_system, "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "M3DUtil/M3UJoint.cpp"),
            Object(Equivalent, "M3DUtil/M3UModel.cpp"),
            Object(Equivalent, "M3DUtil/MActor.cpp"),
            Object(Equivalent, "M3DUtil/MActorAnm.cpp"),
            Object(Equivalent, "M3DUtil/MActorData.cpp"),
            Object(Equivalent, "M3DUtil/SDLModel.cpp"),
            Object(Equivalent, "M3DUtil/MActorUtil.cpp", flags=cflags_system),
            Object(Equivalent, "M3DUtil/SampleCtrlNode.cpp"),
            Object(Matching, "M3DUtil/SampleCtrlModel.cpp"),
            Object(Matching, "M3DUtil/MotionBlendCtrl.cpp"),
            Object(Matching, "M3DUtil/LodAnm.cpp"),
        ],
    },
    {
        "lib": "System",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_system, "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Matching, "System/BaseParam.cpp"),
            Object(Equivalent, "System/EmitterViewObj.cpp"),
            Object(Equivalent, "System/EventWatcher.cpp"),
            Object(Equivalent, "System/FlagManager.cpp"),
            Object(Equivalent, "System/GCLogoDir.cpp"),
            Object(Matching, "System/J3DSysFlag.cpp"),
            Object(Equivalent, "System/MarDirector.cpp"),
            Object(Equivalent, "System/MarDirectorDirect.cpp"),
            Object(Equivalent, "System/MarDirectorEvent.cpp"),
            Object(Equivalent, "System/MarDirectorInitECT.cpp"),
            Object(Equivalent, "System/MarDirectorPreEntry.cpp"),
            Object(Equivalent, "System/MarDirectorSetup2.cpp"),
            Object(Matching, "System/marerr.cpp"),
            Object(Equivalent, "System/MarNameRefGen.cpp"),
            Object(Equivalent, "System/MenuDir.cpp"),
            Object(Matching, "System/Params.cpp"),
            Object(Matching, "System/ParamInst.cpp"),
            Object(Equivalent, "System/PerformList.cpp"),
            Object(Equivalent, "System/RenderModeObj.cpp"),
            Object(Equivalent, "System/SnapTimeObj.cpp"),
            Object(Equivalent, "System/TalkCursor.cpp"),
            Object(Matching, "System/TexCache.cpp"),
            Object(Matching, "System/ZBufferCatch.cpp"),
            Object(Equivalent, "System/Application.cpp"),
            Object(Equivalent, "System/ScenarioArchiveName.cpp"),
            Object(Equivalent, "System/MarioGamePad.cpp"),
            Object(Equivalent, "System/StageEventInfo.cpp"),
            Object(Matching, "System/StageUtil.cpp"),
            Object(Matching, "System/Resolution.cpp"),
            Object(Equivalent, "System/PositionHolder.cpp"),
            Object(Matching, "System/ProcessMeter.cpp"),
            Object(Equivalent, "System/TimeRec.cpp"),
            Object(Equivalent, "System/DrawSyncManager.cpp"),
            Object(Matching, "System/THPRender.cpp"),
            Object(Equivalent, "System/MarNameRefGen_BossEnemy.cpp"),
            Object(Equivalent, "System/MarNameRefGen_Enemy.cpp"),
            Object(Equivalent, "System/MarNameRefGen_Map.cpp"),
            Object(Equivalent, "System/MarNameRefGen_MapObj.cpp"),
            Object(Equivalent, "System/MarNameRefGen_NPC.cpp"),
            Object(Equivalent, "System/CardManager.cpp"),
            Object(Equivalent, "System/MarDirectorLoadResource.cpp"),
            Object(Equivalent, "System/MovieDirector.cpp"),
            Object(Matching, "System/MarDirectorCreateObjects.cpp"),
            Object(Equivalent, "System/MarDirectorSetupObjects.cpp"),
            Object(Equivalent, "System/MSoundMainSide.cpp"),
            Object(Matching, "System/TargetArrow.cpp"),
        ],
    },
    {
        "lib": "Strategic",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Strategic/liveactor.cpp"),
            Object(Equivalent, "Strategic/liveinterp.cpp"),
            Object(Equivalent, "Strategic/livemanager.cpp"),
            Object(Equivalent, "Strategic/ObjHitCheck.cpp"),
            Object(Equivalent, "Strategic/objmanager.cpp"),
            Object(Equivalent, "Strategic/ObjModel.cpp"),
            Object(Equivalent, "Strategic/spcinterp.cpp"),
            Object(Equivalent, "Strategic/Strategy.cpp"),
            Object(Equivalent, "Strategic/question.cpp"),
            Object(Equivalent, "Strategic/smplcharacter.cpp"),
            Object(Equivalent, "Strategic/HitActor.cpp"),
            Object(Matching, "Strategic/binder.cpp"),
            Object(Equivalent, "Strategic/SharedParts.cpp"),
            Object(Equivalent, "Strategic/MirrorActor.cpp"),
        ],
    },
    {
        "lib": "Player",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Player/Atom.cpp"),
            Object(Equivalent, "Player/MarioAction.cpp"),
            Object(Equivalent, "Player/MarioAutodemo.cpp"),
            Object(Equivalent, "Player/MarioBlend.cpp"),
            Object(Equivalent, "Player/MarioCap.cpp"),
            Object(Equivalent, "Player/MarioCollision.cpp"),
            Object(Equivalent, "Player/MarioDraw.cpp"),
            Object(Equivalent, "Player/MarioJump.cpp"),
            Object(Equivalent, "Player/MarioMain.cpp"),
            Object(Equivalent, "Player/MarioMove.cpp"),
            Object(Equivalent, "Player/MarioPhysics.cpp"),
            Object(Matching, "Player/MarioRecord.cpp"),
            Object(Equivalent, "Player/MarioRun.cpp"),
            Object(Equivalent, "Player/MarioSpecial.cpp"),
            Object(Equivalent, "Player/MarioUpper.cpp"),
            Object(Equivalent, "Player/MarioParticle.cpp"),
            Object(Equivalent, "Player/MarioWait.cpp"),
            Object(Equivalent, "Player/SplashManager.cpp"),
            Object(Equivalent, "Player/Tongue.cpp"),
            Object(Equivalent, "Player/WaterGun.cpp"),
            Object(Equivalent, "Player/Yoshi.cpp"),
            Object(Equivalent, "Player/MarioEffect.cpp"),
            Object(Equivalent, "Player/MarioSwim.cpp"),
            Object(Equivalent, "Player/MarioAccess.cpp"),
            Object(Equivalent, "Player/MarioInit.cpp"),
            Object(Equivalent, "Player/ModelWaterManager.cpp"),
            Object(Equivalent, "Player/MarioPositionObj.cpp"),
            Object(Equivalent, "Player/MarioCheckCol.cpp"),
            Object(Equivalent, "Player/MarioReceiveMsg.cpp"),
            Object(Equivalent, "Player/MarioSound.cpp"),
        ],
    },
    {
        "lib": "NPC",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "NPC/NpcAnm.cpp"),
            Object(Equivalent, "NPC/NpcBase.cpp"),
            Object(Equivalent, "NPC/NpcCallback.cpp"),
            Object(Equivalent, "NPC/NpcManager.cpp"),
            Object(Equivalent, "NPC/NpcNerve.cpp"),
            Object(Equivalent, "NPC/NpcSave.cpp"),
            Object(Equivalent, "NPC/NpcEvent.cpp"),
            Object(Equivalent, "NPC/NpcInitData.cpp"),
            Object(Equivalent, "NPC/NpcInitPrg.cpp"),
            Object(Equivalent, "NPC/NpcInbetween.cpp"),
            Object(Equivalent, "NPC/NpcParts.cpp"),
            Object(Equivalent, "NPC/NpcColor.cpp"),
            Object(Equivalent, "NPC/NpcSound.cpp"),
            Object(Equivalent, "NPC/NpcChange.cpp"),
            Object(Equivalent, "NPC/NpcThrow.cpp"),
            Object(Equivalent, "NPC/NpcTrample.cpp"),
            Object(Equivalent, "NPC/NpcEffect.cpp"),
            Object(Equivalent, "NPC/NpcInitAnmData.cpp"),
            Object(Equivalent, "NPC/NpcInitActionData.cpp"),
            Object(Equivalent, "NPC/NpcCoin.cpp"),
            Object(Equivalent, "NPC/NpcBalloon.cpp"),
            Object(Equivalent, "NPC/NpcWalkTurn.cpp"),
            Object(Equivalent, "NPC/NpcCollision.cpp"),

        ],
    },
    {
        "lib": "MSound",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "MSound/MAnmSound.cpp"),
            Object(Equivalent, "MSound/MSound.cpp"),
            Object(Equivalent, "MSound/MSoundBGM.cpp"),
            Object(Matching, "MSound/MSoundDebug.cpp"),
            Object(Equivalent, "MSound/MSoundScene.cpp"),
            Object(Equivalent, "MSound/MSoundSE.cpp"),
            Object(Equivalent, "MSound/MSoundStruct.cpp"),
            Object(Equivalent, "MSound/MSHandle.cpp"),
            Object(Equivalent, "MSound/MSModBgm.cpp"),
        ],
    },
    {
        "lib": "MoveBG",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-opt all,nostrength", "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "MoveBG/WoodBarrel.cpp"),
            Object(Equivalent, "MoveBG/MapObjBase.cpp"),
            Object(Equivalent, "MoveBG/MapObjInit.cpp"),
            Object(Equivalent, "MoveBG/MapObjGeneral.cpp"),
            Object(Equivalent, "MoveBG/MapObjManager.cpp"),
            Object(Equivalent, "MoveBG/MapObjLib.cpp"),
            Object(Equivalent, "MoveBG/Item.cpp"),
            Object(Equivalent, "MoveBG/ItemManager.cpp"),
            Object(Equivalent, "MoveBG/MapObjTown.cpp"),
            Object(Equivalent, "MoveBG/MapObjBlock.cpp"),
            Object(Equivalent, "MoveBG/MapObjBianco.cpp"),
            Object(Equivalent, "MoveBG/MapObjSirena.cpp"),
            Object(Equivalent, "MoveBG/MapObjRicco.cpp"),
            Object(Equivalent, "MoveBG/MapObjMamma.cpp"),
            Object(Equivalent, "MoveBG/MapObjPinna.cpp"),
            Object(Equivalent, "MoveBG/MapObjSample.cpp"),
            Object(Equivalent, "MoveBG/MapObjMare.cpp"),
            Object(Equivalent, "MoveBG/MapObjFlag.cpp"),
            Object(Equivalent, "MoveBG/MapObjWave.cpp"),
            Object(Equivalent, "MoveBG/MapObjFloat.cpp"),
            Object(Equivalent, "MoveBG/MapObjPlane.cpp"),
            Object(Equivalent, "MoveBG/MapObjCloud.cpp"),
            Object(Equivalent, "MoveBG/MapObjBall.cpp"),
            Object(Equivalent, "MoveBG/MapObjAirport.cpp"),
            Object(Equivalent, "MoveBG/MapObjDolpic.cpp"),
            Object(Equivalent, "MoveBG/MapObjPollution.cpp"),
            Object(Equivalent, "MoveBG/MapObjGrass.cpp"),
            Object(Equivalent, "MoveBG/MapObjPole.cpp"),
            Object(Equivalent, "MoveBG/MapObjWater.cpp"),
            Object(Equivalent, "MoveBG/ModelGate.cpp"),
            Object(Equivalent, "MoveBG/MapObjFence.cpp"),
            Object(Equivalent, "MoveBG/MapObjOption.cpp"),
            Object(Equivalent, "MoveBG/MapObjRailBlock.cpp"),
            Object(Equivalent, "MoveBG/MapObjMonte.cpp"),
            Object(Equivalent, "MoveBG/MapObjTree.cpp"),
            Object(Equivalent, "MoveBG/MapObjTumblePole.cpp"),
            Object(Equivalent, "MoveBG/MapObjEx.cpp"),
            Object(Equivalent, "MoveBG/Pool.cpp"),
            Object(Equivalent, "MoveBG/MapObjCorona.cpp"),
            Object(Equivalent, "MoveBG/MapObjItem2.cpp"),
            Object(Equivalent, "MoveBG/MapObjHide.cpp"),
            Object(Equivalent, "MoveBG/MapObjTrap.cpp"),
        ],
    },
    {
        "lib": "Map",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-opt all,nostrength", "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Map/JointModel.cpp"),
            Object(Equivalent, "Map/JointModelManager.cpp"),
            Object(Equivalent, "Map/JointObj.cpp"),
            Object(Equivalent, "Map/Map.cpp"),
            Object(Equivalent, "Map/MapArea.cpp"),
            Object(Equivalent, "Map/MapCheck.cpp"),
            Object(Equivalent, "Map/MapCollisionData.cpp"),
            Object(Equivalent, "Map/MapCollisionEntry.cpp"),
            Object(Equivalent, "Map/MapCollisionManager.cpp"),
            Object(Equivalent, "Map/MapDraw.cpp"),
            Object(Equivalent, "Map/MapEvent.cpp"),
            Object(Equivalent, "Map/MapEventSink.cpp"),
            Object(Equivalent, "Map/MapMakeData.cpp"),
            Object(Equivalent, "Map/MapMakeList.cpp"),
            Object(Equivalent, "Map/MapMirror.cpp"),
            Object(Equivalent, "Map/MapModel.cpp"),
            Object(Equivalent, "Map/MapWarp.cpp"),
            Object(Equivalent, "Map/MapStaticObject.cpp"),
            Object(Equivalent, "Map/MapWire.cpp"),
            Object(Equivalent, "Map/MapWireManager.cpp"),
            Object(Equivalent, "Map/MapXlu.cpp"),
            Object(Equivalent, "Map/PollutionAction.cpp"),
            Object(Equivalent, "Map/PollutionCount.cpp"),
            Object(Equivalent, "Map/PollutionManager.cpp"),
            Object(Equivalent, "Map/PollutionObj.cpp"),
            Object(Equivalent, "Map/PollutionPos.cpp"),
            Object(Equivalent, "Map/Shimmer.cpp"),
            Object(Equivalent, "Map/Sky.cpp"),
            Object(Equivalent, "Map/MapEventSirena.cpp"),
            Object(Equivalent, "Map/PollutionLayer.cpp"),
            Object(Equivalent, "Map/PollutionEvent.cpp"),
            Object(Equivalent, "Map/MapCollisionPlane.cpp"),
            Object(Equivalent, "Map/MarineSnow.cpp"),
            Object(Equivalent, "Map/MapData.cpp"),
            Object(Equivalent, "Map/MapEventDolpic.cpp"),
            Object(Equivalent, "Map/MapEventMare.cpp"),
            Object(Equivalent, "Map/BathWaterManager.cpp"),
            Object(Equivalent, "Map/StickyStainManager.cpp"),
        ],
    },
    {
        "lib": "GC2D",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-opt all,nostrength", "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Matching, "GC2D/ChangeValue.cpp"),
            Object(Matching, "GC2D/Coord2D.cpp"),
            Object(Matching, "GC2D/ExPane.cpp"),
            Object(Equivalent, "GC2D/Menu.cpp"),
            Object(Equivalent, "GC2D/ScrnFader.cpp"),
            Object(Equivalent, "GC2D/GCConsole2.cpp"),
            Object(Equivalent, "GC2D/Talk2D2.cpp"),
            Object(Equivalent, "GC2D/BoundPane.cpp"),
            Object(Equivalent, "GC2D/PauseMenu2.cpp"),
            Object(Equivalent, "GC2D/MessageLoader.cpp"),
            Object(Matching, "GC2D/HelpActor.cpp"),
            Object(Matching, "GC2D/MessageUtil.cpp"),
            Object(Equivalent, "GC2D/CardSave.cpp"),
            Object(Equivalent, "GC2D/CardLoad.cpp"),
            Object(Equivalent, "GC2D/ConsoleStr.cpp"),
            Object(Equivalent, "GC2D/SelectMenu.cpp"),
            Object(Equivalent, "GC2D/SelectDir.cpp"),
            Object(Equivalent, "GC2D/SelectShine2.cpp"),
            Object(Equivalent, "GC2D/BlendPane.cpp"),
            Object(Equivalent, "GC2D/Guide.cpp"),
            Object(Equivalent, "GC2D/SunGlass.cpp"),
            Object(Equivalent, "GC2D/ShineFader.cpp"),
            Object(Equivalent, "GC2D/ProgSelect.cpp"),
            Object(Equivalent, "GC2D/hx_wiper.c"),
            Object(Equivalent, "GC2D/MovieSubtitle.cpp"),
            Object(Equivalent, "GC2D/Option.cpp"),
            Object(Equivalent, "GC2D/MovieRumble.cpp"),
        ],
    },
    {
        "lib": "Enemy",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-opt all,nostrength", "-inline deferred"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Enemy/conductor.cpp"),
            Object(Equivalent, "Enemy/effectObj.cpp"),
            Object(Equivalent, "Enemy/emario.cpp"),
            Object(Equivalent, "Enemy/enemy.cpp"),
            Object(Equivalent, "Enemy/enemyAttachment.cpp"),
            Object(Equivalent, "Enemy/enemymanager.cpp"),
            Object(Equivalent, "Enemy/enemyMario.cpp"),
            Object(Equivalent, "Enemy/feetinv.cpp"),
            Object(Equivalent, "Enemy/gesso.cpp"),
            Object(Equivalent, "Enemy/graph.cpp"),
            Object(Equivalent, "Enemy/hamukuri.cpp"),
            Object(Equivalent, "Enemy/hinokuri2.cpp"),
            Object(Equivalent, "Enemy/mameGesso.cpp"),
            Object(Equivalent, "Enemy/namekuri.cpp"),
            Object(Equivalent, "Enemy/pakkun.cpp"),
            Object(Equivalent, "Enemy/smallEnemy.cpp"),
            Object(Equivalent, "Enemy/spider.cpp"),
            Object(Equivalent, "Enemy/spline.cpp"),
            Object(Equivalent, "Enemy/typicalenemy.cpp"),
            Object(Equivalent, "Enemy/walker.cpp"),
            Object(Equivalent, "Enemy/walkerEnemy.cpp"),
            Object(Equivalent, "Enemy/bossgesso.cpp"),
            Object(Equivalent, "Enemy/elecNokonoko.cpp"),
            Object(Equivalent, "Enemy/telesa.cpp"),
            Object(Equivalent, "Enemy/fireWanwan.cpp"),
            Object(Equivalent, "Enemy/enemytable.cpp"),
            Object(Equivalent, "Enemy/generator.cpp"),
            Object(Equivalent, "Enemy/bosspakkun.cpp"),
            Object(Equivalent, "Enemy/tobiPuku.cpp"),
            Object(Equivalent, "Enemy/tinkoopa.cpp"),
            Object(Equivalent, "Enemy/launcher.cpp"),
            Object(Equivalent, "Enemy/bosswanwan.cpp"),
            Object(Equivalent, "Enemy/chuuhana.cpp"),
            Object(Equivalent, "Enemy/igaiga.cpp"),
            Object(Equivalent, "Enemy/poihana.cpp"),
            Object(Equivalent, "Enemy/tamaNoko.cpp"),
            Object(Equivalent, "Enemy/bosstelesa.cpp"),
            Object(Equivalent, "Enemy/riccohook.cpp"),
            Object(Equivalent, "Enemy/bombhei.cpp"),
            Object(Equivalent, "Enemy/cannon.cpp"),
            Object(Equivalent, "Enemy/bosseel.cpp"),
            Object(Equivalent, "Enemy/killer.cpp"),
            Object(Equivalent, "Enemy/beam.cpp"),
            Object(Equivalent, "Enemy/hanasambo.cpp"),
            Object(Equivalent, "Enemy/popo.cpp"),
            Object(Equivalent, "Enemy/SleepBossHanachan.cpp"),
            Object(Equivalent, "Enemy/DemoBossHanachanBase.cpp"),
            Object(Equivalent, "Enemy/fruitsboat.cpp"),
            Object(Equivalent, "Enemy/BossHanachanSub.cpp"),
            Object(Equivalent, "Enemy/BossHanachanMain.cpp"),
            Object(Equivalent, "Enemy/BossHanachanNerve.cpp"),
            Object(Equivalent, "Enemy/BossHanachanAnm.cpp"),
            Object(Equivalent, "Enemy/BossHanachanParts.cpp"),
            Object(Equivalent, "Enemy/BossHanachanSave.cpp"),
            Object(Equivalent, "Enemy/amiNoko.cpp"),
            Object(Equivalent, "Enemy/gatekeeper.cpp"),
            Object(Equivalent, "Enemy/BossHanachanEffect.cpp"),
            Object(Equivalent, "Enemy/egggen.cpp"),
            Object(Equivalent, "Enemy/seal.cpp"),
            Object(Equivalent, "Enemy/bgpoldrop.cpp"),
            Object(Equivalent, "Enemy/bgtentacle.cpp"),
            Object(Equivalent, "Enemy/effectEnemy.cpp"),
            Object(Equivalent, "Enemy/hauntLeg.cpp"),
            Object(Equivalent, "Enemy/areacylinder.cpp"),
            Object(Equivalent, "Enemy/wireTrap.cpp"),
            Object(Equivalent, "Enemy/BossHanachanSound.cpp"),
            Object(Equivalent, "Enemy/rocket.cpp"),
            Object(Equivalent, "Enemy/Kazekun.cpp"),
            Object(Equivalent, "Enemy/bossManta.cpp"),
            Object(Equivalent, "Enemy/wireBinder.cpp"),
            Object(Equivalent, "Enemy/yunbo.cpp"),
            Object(Equivalent, "Enemy/koopajr.cpp"),
            Object(Equivalent, "Enemy/Kumokun.cpp"),
            Object(Equivalent, "Enemy/Koopa.cpp"),
            Object(Equivalent, "Enemy/Kukku.cpp"),
            Object(Equivalent, "Enemy/Amenbo.cpp"),
            Object(Equivalent, "Enemy/BathtubPeach.cpp"),
            Object(Equivalent, "Enemy/BathtubKiller.cpp"),
            Object(Equivalent, "Enemy/coasterkiller.cpp"),
            Object(Equivalent, "Enemy/DebuTelesa.cpp"),
            Object(Equivalent, "Enemy/TabePuku.cpp"),
            Object(Equivalent, "Enemy/BathtubBinder.cpp"),
            Object(Equivalent, "Enemy/limitkoopa.cpp"),
            Object(Equivalent, "Enemy/limitkoopajr.cpp"),
        ],
    },
    {
        "lib": "Camera",
        "mw_version": "GC/1.2.5",
        "cflags": [*cflags_game, "-inline deferred", "-opt all,nostrength"],
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Camera/CameraBGCheck.cpp"),
            Object(Equivalent, "Camera/CameraChange.cpp"),
            Object(Equivalent, "Camera/CameraCodeControl.cpp"),
            Object(Equivalent, "Camera/cameragc.cpp"),
            Object(Equivalent, "Camera/CameraHeightPan.cpp"),
            Object(Equivalent, "Camera/CameraInbetween.cpp"),
            Object(Matching, "Camera/CameraKindParam.cpp"),
            Object(Equivalent, "Camera/cameralib.cpp"),
            Object(Equivalent, "Camera/CameraMarioData.cpp"),
            Object(Equivalent, "Camera/CameraNotice.cpp"),
            Object(Equivalent, "Camera/CameraNormal.cpp"),
            Object(Equivalent, "Camera/camerasave.cpp"),
            Object(Equivalent, "Camera/camerashake.cpp"),
            Object(Equivalent, "Camera/CameraTalk.cpp"),
            Object(Equivalent, "Camera/lensflare.cpp"),
            Object(Equivalent, "Camera/lensglow.cpp"),
            Object(Equivalent, "Camera/sunmgr.cpp"),
            Object(Equivalent, "Camera/sunmodel.cpp"),
            Object(Equivalent, "Camera/CubeManagerBase.cpp"),
            Object(Equivalent, "Camera/CameraMapTool.cpp"),
            Object(Equivalent, "Camera/CubeMapTool.cpp"),
            Object(Equivalent, "Camera/CameraMultiPlayer.cpp"),
            Object(Equivalent, "Camera/CameraJetCoaster.cpp"),
            Object(Equivalent, "Camera/CameraBck.cpp"),
            Object(Equivalent, "Camera/CameraOption.cpp"),
            Object(Equivalent, "Camera/CameraDemo.cpp"),
            Object(Equivalent, "Camera/CameraWarp.cpp"),
            Object(Equivalent, "Camera/CameraMode.cpp"),
            Object(Equivalent, "Camera/CameraSecureView.cpp"),
            Object(Matching, "Camera/CamShakeDefine.cpp"),
        ],
    },
    {
        "lib": "Animal",
        "mw_version": "GC/1.2.5",
        "cflags": cflags_game,
        "progress_category": "game",
        "objects": [
            Object(Equivalent, "Animal/boid.cpp"),
            Object(Equivalent, "Animal/fishoid.cpp"),
            Object(Equivalent, "Animal/AnimalBase.cpp"),
            Object(Equivalent, "Animal/AnimalManager.cpp"),
            Object(Equivalent, "Animal/AnimalSave.cpp"),
            Object(Equivalent, "Animal/AnimalNerve.cpp"),
            Object(Equivalent, "Animal/Bird.cpp"),
            Object(Equivalent, "Animal/BeeHive.cpp"),
            Object(Equivalent, "Animal/Butterfly.cpp"),
        ],
    },
]

# Optional extra categories for progress tracking
# Adjust as desired for your project
config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("jsystem", "JSystem Middleware"),
    ProgressCategory("sdk", "SDK Code"),
]
config.progress_each_module = args.verbose

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress and write progress.json
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)
