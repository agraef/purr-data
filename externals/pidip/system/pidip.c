#include <stdio.h>
#include  "pdp.h"
#include  "pidip_config.h"


/* all symbols are C style */
#ifdef __cplusplus
extern "C"
{
#endif

    void pdp_intrusion_setup(void);
    void pdp_simura_setup(void);
    void pdp_underwatch_setup(void);
    void pdp_vertigo_setup(void);
    void pdp_yvu2rgb_setup(void);
    void pdp_yqt_setup(void);
    void pdp_fqt_setup(void);
    void pdp_fcqt_setup(void);
    void pdp_lens_setup(void);
    void pdp_baltan_setup(void);
    void pdp_aging_setup(void);
    void pdp_ripple_setup(void);
    void pdp_warp_setup(void);
    void pdp_rev_setup(void);
    void pdp_mosaic_setup(void);
    void pdp_edge_setup(void);
    void pdp_spiral_setup(void);
    void pdp_radioactiv_setup(void);
    void pdp_warhol_setup(void);
    void pdp_nervous_setup(void);
    void pdp_quark_setup(void);
    void pdp_spigot_setup(void);
    void pdp_rec_tilde_setup(void);
    void pdp_o_setup(void);
    void pdp_i_setup(void);
    void pdp_mgrid_setup(void);
    void pdp_ctrack_setup(void);
    void pdp_cycle_setup(void);
    void pdp_transform_setup(void);
    void pdp_shagadelic_setup(void);
    void pdp_dice_setup(void);
    void pdp_puzzle_setup(void);
    void pdp_text_setup(void);
    void pdp_qtext_setup(void);
    void pdp_form_setup(void);
    void pdp_compose_setup(void);
    void pdp_cmap_setup(void);
    // void pdp_aa_setup(void);
    void pdp_ascii_setup(void);
    void pdp_segsnd_tilde_setup(void);
    void pdp_noquark_setup(void);
    void pdp_juxta_setup(void);
    void pdp_smuck_setup(void);
    void pdp_lumafilt_setup(void);
    void pdp_transition_setup(void);
    void pdp_imgloader_setup(void);
    void pdp_imgsaver_setup(void);
    void pdp_cache_setup(void);
    void pdp_canvas_setup(void);
    // void pdp_xcanvas_setup(void);
    void pdp_ocanvas_setup(void);
    void pdp_pen_setup(void);
    void pdp_shape_setup(void);
    void pdp_spotlight_setup(void);
    void pdp_colorgrid_setup(void);
    void pdp_binary_setup(void);
    void pdp_erode_setup(void);
    void pdp_dilate_setup(void);
    void pdp_hitandmiss_setup(void);
    void pdp_disintegration_setup(void);
    void pdp_distance_setup(void);
    void pdp_theorin_tilde_setup(void);
    void pdp_theorout_tilde_setup(void);
    void pdp_cropper_setup(void);
    void pdp_background_setup(void);
    void pdp_mapper_setup(void);
    void pdp_theonice_tilde_setup(void);
    void pdp_icedthe_tilde_setup(void);
    void pdp_fdiff_setup(void);
    void pdp_hue_setup(void);
    void pdp_dot_setup(void);

#ifdef HAVE_DC1394
    void pdp_dc1394_setup(void);
#endif


#ifdef HAVE_V4L2
    void pdp_v4l2_setup(void);
    void pdp_vloopback_setup(void);
#endif

#ifdef HAVE_LIBDV
    void pdp_ieee1394_setup(void);
#endif

#ifdef __APPLE__
    void pdp_ieee1394_setup(void);
#endif

#ifdef HAVE_IMAGE_MAGICK
    void pdp_capture_setup(void);
#endif

#ifdef HAVE_PIDIP_FFMPEG
    void pdp_ffmpeg_tilde_setup(void);
    void pdp_live_tilde_setup(void);
#endif

#ifdef HAVE_PIDIP_MPEG4IP
    void pdp_mp4live_tilde_setup(void);
    void pdp_mp4player_tilde_setup(void);
#endif

/* library setup routine */
void pidip_setup(void){
    
    post("PiDiP : additional video processing objects for PDP\n\tversion " PDP_PIDIP_VERSION "\n\tby Yves Degoyon and Lluis Gomez i Bigorda");

    pdp_intrusion_setup();
    pdp_yqt_setup();
    pdp_fqt_setup();
    pdp_fcqt_setup();
    pdp_simura_setup();
    pdp_underwatch_setup();
    pdp_vertigo_setup();
    pdp_yvu2rgb_setup();
    pdp_lens_setup();
    pdp_baltan_setup();
    pdp_aging_setup();
    pdp_ripple_setup();
    pdp_warp_setup();
    pdp_rev_setup();
    pdp_mosaic_setup();
    pdp_edge_setup();
    pdp_spiral_setup();
    pdp_radioactiv_setup();
    pdp_warhol_setup();
    pdp_nervous_setup();
    pdp_quark_setup();
    pdp_spigot_setup();
    pdp_rec_tilde_setup();
    pdp_o_setup();
    pdp_i_setup();
    pdp_mgrid_setup();
    pdp_ctrack_setup();
    pdp_cycle_setup();
    pdp_transform_setup();
    pdp_shagadelic_setup();
    pdp_dice_setup();
    pdp_puzzle_setup();
    pdp_text_setup();
    pdp_qtext_setup();
    pdp_form_setup();
    pdp_compose_setup();
    pdp_cmap_setup();
    // pdp_aa_setup();
    pdp_ascii_setup();
    pdp_segsnd_tilde_setup();
    pdp_noquark_setup();
    pdp_juxta_setup();
    pdp_smuck_setup();
    pdp_lumafilt_setup();
    pdp_transition_setup();
    pdp_imgloader_setup();
    pdp_imgsaver_setup();
    pdp_cache_setup();
    pdp_canvas_setup();
    // pdp_xcanvas_setup();
    pdp_ocanvas_setup();
    pdp_pen_setup();
    pdp_shape_setup();
    pdp_spotlight_setup();
    pdp_colorgrid_setup();
    pdp_binary_setup();
    pdp_erode_setup();
    pdp_dilate_setup();
    pdp_hitandmiss_setup();
    pdp_disintegration_setup();
    pdp_distance_setup();
    pdp_theorin_tilde_setup();
    pdp_theorout_tilde_setup();
    pdp_cropper_setup();
    pdp_background_setup();
    pdp_mapper_setup();
    pdp_theonice_tilde_setup();
    pdp_icedthe_tilde_setup();
    pdp_fdiff_setup();
    pdp_hue_setup();
    pdp_dot_setup();

#ifdef HAVE_DC1394
    pdp_dc1394_setup();
#endif

#ifdef HAVE_V4L2
    pdp_v4l2_setup();
    pdp_vloopback_setup();
#endif

#ifdef HAVE_LIBDV
    pdp_ieee1394_setup();
#endif

#ifdef __APPLE__
       pdp_ieee1394_setup();
#endif

#ifdef HAVE_IMAGE_MAGICK
    pdp_capture_setup();
#endif

#ifdef HAVE_PIDIP_FFMPEG
    pdp_ffmpeg_tilde_setup();
    pdp_live_tilde_setup();
#endif

#ifdef HAVE_PIDIP_MPEG4IP
    pdp_mp4live_tilde_setup();
    pdp_mp4player_tilde_setup();
#endif
}

#ifdef __cplusplus
}
#endif
