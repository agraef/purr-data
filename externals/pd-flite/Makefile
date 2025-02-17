# Makefile to build class 'helloworld' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = flite


SOURCE_DIR = ./flite-master

XINCLUDE = -I ${SOURCE_DIR}/include \
	 -I ${SOURCE_DIR}/lang/cmulex \
	 -I ${SOURCE_DIR}/lang/cmu_grapheme_lang \
	 -I ${SOURCE_DIR}/lang/cmu_grapheme_lex \
	 -I ${SOURCE_DIR}/lang/cmu_indic_lang \
	 -I ${SOURCE_DIR}/lang/cmu_indic_lex \
	 -I ${SOURCE_DIR}/lang/cmu_time_awb \
	 -I ${SOURCE_DIR}/lang/cmu_us_awb \
	 -I ${SOURCE_DIR}/lang/cmu_us_kal \
	 -I ${SOURCE_DIR}/lang/cmu_us_rms \
	 -I ${SOURCE_DIR}/lang/cmu_us_slt \
	 -I ${SOURCE_DIR}/lang/usenglish \
	 -I ${SOURCE_DIR}/main \
	 -I ${SOURCE_DIR}/sapi/FliteTTSEngineObj \
	 -I ${SOURCE_DIR}/src/audio \
	 -I ${SOURCE_DIR}/src/cg \
	 -I ${SOURCE_DIR}/src/hrg \
	 -I ${SOURCE_DIR}/src/lexicon \
	 -I ${SOURCE_DIR}/src/regex \
	 -I ${SOURCE_DIR}/src/speech \
	 -I ${SOURCE_DIR}/src/stats \
	 -I ${SOURCE_DIR}/src/synth \
	 -I ${SOURCE_DIR}/src/utils \
	 -I ${SOURCE_DIR}/src/wavesynth \
	 -I ${SOURCE_DIR}/testsuite \
	 -I ${SOURCE_DIR}/tools \
	 -I ${SOURCE_DIR}/wince \
	 ${empty}
	 

cflags = ${XINCLUDE} -I . -DVERSION='"0.3.2"'
ldlibs += -lm -lpthread


# input source file (class name == source file basename)
flite.class.sources = flite.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_data.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_entries.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lts_model.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lts_rules.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_postlex.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lang/cmu_grapheme_lang.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lang/cmu_grapheme_phoneset.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lang/cmu_grapheme_phrasing_cart.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lang/graph_gpos.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lex/cmu_grapheme_lex.c \
	 ${SOURCE_DIR}/lang/cmu_grapheme_lex/grapheme_unitran_tables.c \
	 ${SOURCE_DIR}/lang/cmu_indic_lang/cmu_indic_lang.c \
	 ${SOURCE_DIR}/lang/cmu_indic_lang/cmu_indic_phoneset.c \
	 ${SOURCE_DIR}/lang/cmu_indic_lang/cmu_indic_phrasing_cart.c \
	 ${SOURCE_DIR}/lang/cmu_indic_lex/cmu_indic_lex.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb_cart.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb_clunits.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb_lex_entry.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb_lpc.c \
	 ${SOURCE_DIR}/lang/cmu_time_awb/cmu_time_awb_mcep.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg_durmodel.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg_f0_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg_phonestate.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg_single_mcep_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_cg_single_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_spamf0_accent.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_spamf0_accent_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_awb/cmu_us_awb_spamf0_phrase.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal_diphone.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal_lpc.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal_res.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal_residx.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal/cmu_us_kal_ressize.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal16/cmu_us_kal16.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal16/cmu_us_kal16_diphone.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal16/cmu_us_kal16_lpc.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal16/cmu_us_kal16_res.c \
	 ${SOURCE_DIR}/lang/cmu_us_kal16/cmu_us_kal16_residx.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg_durmodel.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg_f0_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg_phonestate.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg_single_mcep_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_cg_single_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_spamf0_accent.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_spamf0_accent_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_rms/cmu_us_rms_spamf0_phrase.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg_durmodel.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg_f0_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg_phonestate.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg_single_mcep_trees.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_cg_single_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_spamf0_accent.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_spamf0_accent_params.c \
	 ${SOURCE_DIR}/lang/cmu_us_slt/cmu_us_slt_spamf0_phrase.c \
	 ${SOURCE_DIR}/lang/usenglish/usenglish.c \
	 ${SOURCE_DIR}/lang/usenglish/us_aswd.c \
	 ${SOURCE_DIR}/lang/usenglish/us_durz_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_dur_stats.c \
	 ${SOURCE_DIR}/lang/usenglish/us_expand.c \
	 ${SOURCE_DIR}/lang/usenglish/us_f0lr.c \
	 ${SOURCE_DIR}/lang/usenglish/us_f0_model.c \
	 ${SOURCE_DIR}/lang/usenglish/us_ffeatures.c \
	 ${SOURCE_DIR}/lang/usenglish/us_gpos.c \
	 ${SOURCE_DIR}/lang/usenglish/us_int_accent_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_int_tone_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_nums_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_phoneset.c \
	 ${SOURCE_DIR}/lang/usenglish/us_phrasing_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_pos_cart.c \
	 ${SOURCE_DIR}/lang/usenglish/us_text.c \
	 ${SOURCE_DIR}/src/audio/au_none.c \
	 ${SOURCE_DIR}/src/audio/audio.c \
	 ${SOURCE_DIR}/src/audio/au_streaming.c \
	 ${SOURCE_DIR}/src/cg/cst_cg.c \
	 ${SOURCE_DIR}/src/cg/cst_cg_dump_voice.c \
	 ${SOURCE_DIR}/src/cg/cst_cg_load_voice.c \
	 ${SOURCE_DIR}/src/cg/cst_cg_map.c \
	 ${SOURCE_DIR}/src/cg/cst_mlpg.c \
	 ${SOURCE_DIR}/src/cg/cst_mlsa.c \
	 ${SOURCE_DIR}/src/cg/cst_spamf0.c \
	 ${SOURCE_DIR}/src/cg/cst_vc.c \
	 ${SOURCE_DIR}/src/hrg/cst_ffeature.c \
	 ${SOURCE_DIR}/src/hrg/cst_item.c \
	 ${SOURCE_DIR}/src/hrg/cst_relation.c \
	 ${SOURCE_DIR}/src/hrg/cst_rel_io.c \
	 ${SOURCE_DIR}/src/hrg/cst_utterance.c \
	 ${SOURCE_DIR}/src/lexicon/cst_lexicon.c \
	 ${SOURCE_DIR}/src/lexicon/cst_lts.c \
	 ${SOURCE_DIR}/src/lexicon/cst_lts_rewrites.c \
	 ${SOURCE_DIR}/src/regex/cst_regex.c \
	 ${SOURCE_DIR}/src/regex/regexp.c \
	 ${SOURCE_DIR}/src/regex/regsub.c \
	 ${SOURCE_DIR}/src/speech/cst_lpcres.c \
	 ${SOURCE_DIR}/src/speech/cst_track.c \
	 ${SOURCE_DIR}/src/speech/cst_track_io.c \
	 ${SOURCE_DIR}/src/speech/cst_wave.c \
	 ${SOURCE_DIR}/src/speech/cst_wave_io.c \
	 ${SOURCE_DIR}/src/speech/cst_wave_utils.c \
	 ${SOURCE_DIR}/src/speech/g721.c \
	 ${SOURCE_DIR}/src/speech/g723_24.c \
	 ${SOURCE_DIR}/src/speech/g723_40.c \
	 ${SOURCE_DIR}/src/speech/g72x.c \
	 ${SOURCE_DIR}/src/speech/rateconv.c \
	 ${SOURCE_DIR}/src/stats/cst_cart.c \
	 ${SOURCE_DIR}/src/stats/cst_ss.c \
	 ${SOURCE_DIR}/src/stats/cst_viterbi.c \
	 ${SOURCE_DIR}/src/synth/cst_ffeatures.c \
	 ${SOURCE_DIR}/src/synth/cst_phoneset.c \
	 ${SOURCE_DIR}/src/synth/cst_ssml.c \
	 ${SOURCE_DIR}/src/synth/cst_synth.c \
	 ${SOURCE_DIR}/src/synth/cst_utt_utils.c \
	 ${SOURCE_DIR}/src/synth/cst_voice.c \
	 ${SOURCE_DIR}/src/synth/flite.c \
	 ${SOURCE_DIR}/src/utils/cst_alloc.c \
	 ${SOURCE_DIR}/src/utils/cst_args.c \
	 ${SOURCE_DIR}/src/utils/cst_endian.c \
	 ${SOURCE_DIR}/src/utils/cst_error.c \
	 ${SOURCE_DIR}/src/utils/cst_features.c \
	 ${SOURCE_DIR}/src/utils/cst_file_stdio.c \
	 ${SOURCE_DIR}/src/utils/cst_mmap_none.c \
	 ${SOURCE_DIR}/src/utils/cst_socket.c \
	 ${SOURCE_DIR}/src/utils/cst_string.c \
	 ${SOURCE_DIR}/src/utils/cst_tokenstream.c \
	 ${SOURCE_DIR}/src/utils/cst_url.c \
	 ${SOURCE_DIR}/src/utils/cst_val.c \
	 ${SOURCE_DIR}/src/utils/cst_val_const.c \
	 ${SOURCE_DIR}/src/utils/cst_val_user.c \
	 ${SOURCE_DIR}/src/utils/cst_wchar.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_clunits.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_diphone.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_reflpc.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_sigpr.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_sts.c \
	 ${SOURCE_DIR}/src/wavesynth/cst_units.c \
	 ${empty}

# unused sources
EXCLUDEDFILES = \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_data_raw.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_entries_huff_table.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_num_bytes.c \
	 ${SOURCE_DIR}/lang/cmulex/cmu_lex_phones_huff_table.c \
	 ${SOURCE_DIR}/sapi/FliteTTSEngineObj/flite_sapi_usenglish.c \
	 ${SOURCE_DIR}/src/audio/auclient.c \
	 ${SOURCE_DIR}/src/audio/auserver.c \
	 ${SOURCE_DIR}/src/audio/au_alsa.c \
	 ${SOURCE_DIR}/src/audio/au_command.c \
	 ${SOURCE_DIR}/src/audio/au_oss.c \
	 ${SOURCE_DIR}/src/audio/au_palmos.c \
	 ${SOURCE_DIR}/src/audio/au_pulseaudio.c \
	 ${SOURCE_DIR}/src/audio/au_sun.c \
	 ${SOURCE_DIR}/src/audio/au_win.c \
	 ${SOURCE_DIR}/src/audio/au_wince.c \
	 ${SOURCE_DIR}/src/utils/cst_file_palmos.c \
	 ${SOURCE_DIR}/src/utils/cst_file_wince.c \
	 ${SOURCE_DIR}/src/utils/cst_mmap_posix.c \
	 ${SOURCE_DIR}/src/utils/cst_mmap_win32.c \
	 ${SOURCE_DIR}/testsuite/asciiS2U_main.c \
	 ${SOURCE_DIR}/testsuite/asciiU2S_main.c \
	 ${SOURCE_DIR}/testsuite/bin2ascii_main.c \
	 ${SOURCE_DIR}/testsuite/by_word_main.c \
	 ${SOURCE_DIR}/testsuite/combine_waves_main.c \
	 ${SOURCE_DIR}/testsuite/compare_wave_main.c \
	 ${SOURCE_DIR}/testsuite/dcoffset_wave_main.c \
	 ${SOURCE_DIR}/testsuite/flite_test_main.c \
	 ${SOURCE_DIR}/testsuite/hrg_test_main.c \
	 ${SOURCE_DIR}/testsuite/kal_test_main.c \
	 ${SOURCE_DIR}/testsuite/lex_lookup_main.c \
	 ${SOURCE_DIR}/testsuite/lex_test_main.c \
	 ${SOURCE_DIR}/testsuite/lpc_resynth_main.c \
	 ${SOURCE_DIR}/testsuite/lpc_test2_main.c \
	 ${SOURCE_DIR}/testsuite/lpc_test_main.c \
	 ${SOURCE_DIR}/testsuite/multi_thread_main.c \
	 ${SOURCE_DIR}/testsuite/nums_test_main.c \
	 ${SOURCE_DIR}/testsuite/play_client_main.c \
	 ${SOURCE_DIR}/testsuite/play_server_main.c \
	 ${SOURCE_DIR}/testsuite/play_sync_main.c \
	 ${SOURCE_DIR}/testsuite/play_wave_main.c \
	 ${SOURCE_DIR}/testsuite/record_in_noise_main.c \
	 ${SOURCE_DIR}/testsuite/record_wave_main.c \
	 ${SOURCE_DIR}/testsuite/regex_test_main.c \
	 ${SOURCE_DIR}/testsuite/rfc_main.c \
	 ${SOURCE_DIR}/testsuite/token_test_main.c \
	 ${SOURCE_DIR}/testsuite/tris1_main.c \
	 ${SOURCE_DIR}/testsuite/utt_test_main.c \
	 ${SOURCE_DIR}/tools/find_sts_main.c \
	 ${SOURCE_DIR}/tools/flite_sort_main.c \
	 ${SOURCE_DIR}/tools/LANGNAME_lang.c \
	 ${SOURCE_DIR}/tools/LANGNAME_lex.c \
	 ${SOURCE_DIR}/tools/VOICE_cg.c \
	 ${SOURCE_DIR}/tools/VOICE_clunits.c \
	 ${SOURCE_DIR}/tools/VOICE_diphone.c \
	 ${SOURCE_DIR}/tools/VOICE_ldom.c \
	 ${SOURCE_DIR}/wince/flowm_flite.c \
	 ${SOURCE_DIR}/wince/flowm_main.c \
	 ${SOURCE_DIR}/main/compile_regexes.c \
	 ${SOURCE_DIR}/main/flitevox_info_main.c \
	 ${SOURCE_DIR}/main/flite_lang_list.c \
	 ${SOURCE_DIR}/main/flite_main.c \
	 ${SOURCE_DIR}/main/flite_time_main.c \
	 ${SOURCE_DIR}/main/t2p_main.c \
	 ${SOURCE_DIR}/main/word_times_main.c \
	 ${empty}
	 
	 
define forWindows
  XINCLUDE += \
	-DCST_NO_SOCKETS \
	-DUNDER_WINDOWS \
	-DWIN32 \
    ${empty}
  ldlibs += 
endef

define forDarwin
  XINCLUDE += \
	-DCST_AUDIO_NONE \
	-no-cpp-precomp \
    ${empty}
endef

# all extra files to be included in binary distribution of the library
datafiles = \
	flite-help.pd \
	flite-numbers.pd flite-test2.pd flite-test.pd \
	README.md flite-meta.pd CHANGELOG.txt \
	

#alldebug: CPPFLAGS+=-DFLITE_DEBUG=1


# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/flite.${extension}"
