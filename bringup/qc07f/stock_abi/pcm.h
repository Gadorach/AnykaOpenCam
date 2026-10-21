/* SPDX-License-Identifier: GPL-2.0-only
 * REA-observed retail capture/playback ABI reference, not an ALSA driver.
 * No device operations or audio reconfiguration are performed by this header.
 */
#ifndef QC07F_STOCK_PCM_ABI_H
#define QC07F_STOCK_PCM_ABI_H
#include <stdint.h>
#define QC07F_PCM_CAPTURE_PATH "/dev/akpcm_cdev1"
#define QC07F_PCM_PLAYBACK_PATH "/dev/akpcm_cdev0"
#define QC07F_PCM_SET_PARAMS_WIRE 0x401c50f2UL
#define QC07F_PCM_GET_PARAMS_WIRE 0x801c50f2UL
#define QC07F_PCM_GET_CAPS_WIRE 0x804050f0UL
#define QC07F_PCM_GET_ECHO_PARAMS_WIRE 0x80045085UL
#define QC07F_PCM_SET_ECHO_PARAMS_WIRE 0x40045086UL
struct qc07f_pcm_params32_wire { uint32_t words[7]; };
struct qc07f_pcm_caps32_wire { uint32_t words[16]; };
struct qc07f_pcm_echo32_wire { uint32_t words[10]; };
_Static_assert(sizeof(struct qc07f_pcm_params32_wire)==28,"retail params copy");
_Static_assert(sizeof(struct qc07f_pcm_caps32_wire)==64,"retail caps copy");
_Static_assert(sizeof(struct qc07f_pcm_echo32_wire)==40,"retail echo copy");
/* GET/SET_ECHO_PARAMS encode4 bytes but retail capture ioctl copies40 bytes.
 * Never size a buffer using only the encoded ioctl size. Parameter word0 is
 * passed to codec samplerate setup; period count word4 is checked <=80 and
 * threshold word5 may be raised to3*word3. Remaining field semantics need
 * typed source/runtime correlation, not guessed struct names.
 * Open-source replacement boundary: ALSA/ASoC PCM + validated proprietary-L2
 * DMA service; standard open codecs/DSP may replace the runtime codec/filter
 * libraries. No claim of legacy binary ABI compatibility with Linux6.
 */
#endif
