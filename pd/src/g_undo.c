#include "m_pd.h"
#include "g_canvas.h"
#include <stdio.h>
#include "g_undo.h"

/* used for canvas_objtext to differentiate between objects being created
   by user vs. those (re)created by the undo/redo actions */

int we_are_undoing = 0;

extern const char *canvas_undo_name;
extern void glob_preset_node_list_seek_hub(void);
extern void glob_preset_node_list_check_loc_and_update(void);
extern void text_checkvalidwidth(t_glist *glist);

t_undo_action *canvas_undo_init(t_canvas *x)
{
    t_undo_action *a = (t_undo_action *)getbytes(sizeof(*a));

    a->type = 0;
    a->x = x;
    a->next = NULL;

    if (!x->u_queue)
    {
        //this is the first init
        x->u_queue = a;
        x->u_last = a;
        a->prev = NULL;
        a->name = "no";
        gui_vmess("gui_undo_menu", "xss", (t_int)a->x, "no", "no");
    }
    else
    {
        if (x->u_last->next)
        {
            //we need to rebranch first then add the new action
            canvas_undo_rebranch(x);
        }
        x->u_last->next = a;
        a->prev = x->u_last;
        x->u_last = a;
    } 
    //fprintf(stderr,"canvas_undo_init\n");
    return(a);
}

t_undo_action *canvas_undo_add(t_canvas *x, int type, const char *name,
    void *data)
{
    //fprintf(stderr,"canvas_undo_add %d\n", type);

    if(UNDO_SEQUENCE_END == type
       && x && x->u_last
       && UNDO_SEQUENCE_START == x->u_last->type)
    {
            /* empty undo sequence...get rid of it */
        x->u_last = x->u_last->prev;
        canvas_undo_rebranch(x);
        x->u_last->next = 0;
        canvas_undo_name = x->u_last->name;
        gui_vmess("gui_undo_menu", "xss", x, x->u_last->name, "no");
        return 0;
    }

    t_undo_action *a = canvas_undo_init(x);
    a->type = type;
    a->data = (void *)data;
    a->name = (char *)name;
    canvas_undo_name = name;
    gui_vmess("gui_undo_menu", "xss", x, a->name, "no");
    return(a);
}

static void canvas_undo_doit(t_canvas *x, t_undo_action *udo, int action)
{
    switch(udo->type)
    {
    case UNDO_CONNECT:      canvas_undo_connect(x, udo->data, action); break;     //connect
    case UNDO_DISCONNECT:   canvas_undo_disconnect(x, udo->data, action); break;  //disconnect
    case UNDO_CUT:          canvas_undo_cut(x, udo->data, action); break;         //cut
    case UNDO_MOTION:       canvas_undo_move(x, udo->data, action); break;        //move
    case UNDO_PASTE:        canvas_undo_paste(x, udo->data, action); break;       //paste
    case UNDO_APPLY:        canvas_undo_apply(x, udo->data, action); break;       //apply
    case UNDO_ARRANGE:      canvas_undo_arrange(x, udo->data, action); break;     //arrange
    case UNDO_CANVAS_APPLY: canvas_undo_canvas_apply(x, udo->data, action); break;//canvas apply
    case UNDO_CREATE:       canvas_undo_create(x, udo->data, action); break;      //create
    case UNDO_RECREATE:     canvas_undo_recreate(x, udo->data, action); break;    //recreate
    case UNDO_FONT:         canvas_undo_font(x, udo->data, action); break;        //font
            /* undo sequences are handled in canvas_undo_undo resp canvas_undo_redo */
    case UNDO_SEQUENCE_START: break;                                            //start undo sequence
    case UNDO_SEQUENCE_END: break;                                              //end undo sequence
    case UNDO_INIT: /* catch whether is called with a non FREE action */ break; //init
    default:
        error("canvas_undo: unsupported command %d", udo->type);
    }
}

void canvas_undo_undo(t_canvas *x)
{
    int dspwas = canvas_suspend_dsp();
    if (x->u_queue && x->u_last != x->u_queue)
    {
        we_are_undoing = 1;
        //fprintf(stderr,"canvas_undo_undo %d\n", x->u_last->type);
        canvas_editmode(x, 1);
        glist_noselect(x);
        canvas_undo_name = x->u_last->name;

        if(UNDO_SEQUENCE_END == x->u_last->type)
        {
            int sequence_depth = 1;
            if(x->u_last->data) post("undo info: %s", (char *)x->u_last->data);
            while((x->u_last = x->u_last->prev)
                  && (UNDO_INIT != x->u_last->type))
            {
                switch(x->u_last->type)
                {
                case UNDO_SEQUENCE_START:
                    sequence_depth--;
                    break;
                case UNDO_SEQUENCE_END:
                    sequence_depth++;
                    if(x->u_last->data) post("undo info: %s", (char *)x->u_last->data);
                    break;
                default:
                    canvas_undo_doit(x, x->u_last, UNDO_UNDO);
                }
                if (sequence_depth < 1)
                    break;
            }
            if (sequence_depth < 0)
                bug("undo sequence missing end");
            else if (sequence_depth > 0)
                bug("undo sequence missing start");
        }

        /* prevent from crashing if there was no sequence starting point */
        if(x->u_last != x->u_queue)
        {
            canvas_undo_doit(x, x->u_last, UNDO_UNDO);
            x->u_last = x->u_last->prev;
        }

        char *undo_action = x->u_last->name;
        char *redo_action = x->u_last->next->name;
        we_are_undoing = 0;

        /* here we call updating of all unpaired hubs and nodes since
        their regular call will fail in case their position needed
        to be updated by undo/redo first to reflect the old one */
        glob_preset_node_list_seek_hub();
        glob_preset_node_list_check_loc_and_update();
        if (glist_isvisible(x) && glist_istoplevel(x))
        {
            gui_vmess("gui_undo_menu", "xss",
                x, undo_action, redo_action);
            text_checkvalidwidth(x);
            canvas_getscroll(x);
        }
        canvas_dirty(x, 1);
    }
    canvas_resume_dsp(dspwas);
}

void canvas_undo_redo(t_canvas *x)
{
    int dspwas = canvas_suspend_dsp();
    if (x->u_queue && x->u_last->next)
    {
        we_are_undoing = 1;
        x->u_last = x->u_last->next;
        //fprintf(stderr,"canvas_undo_redo %d\n", x->u_last->type);
        canvas_editmode(x, 1);
        glist_noselect(x);
        canvas_undo_name = x->u_last->name;

        if(UNDO_SEQUENCE_START == x->u_last->type)
        {
            int sequence_depth = 1;
            if(x->u_last->data) post("redo info: %s", (char *)x->u_last->data);
            while(x->u_last->next && (x->u_last = x->u_last->next))
            {
                switch(x->u_last->type)
                {
                case UNDO_SEQUENCE_END:
                    sequence_depth--;
                    break;
                case UNDO_SEQUENCE_START:
                    sequence_depth++;
                    if(x->u_last->data) post("redo info: %s", (char *)x->u_last->data);
                    break;
                default:
                    canvas_undo_doit(x, x->u_last, UNDO_REDO);
                }
                if (sequence_depth < 1)
                    break;
            }
            if (sequence_depth < 0)
                bug("undo sequence end without start");
            else if (sequence_depth > 0)
                bug("undo sequence start without end");
        }

        canvas_undo_doit(x, x->u_last, UNDO_REDO);

        char *undo_action = x->u_last->name;
        char *redo_action = (x->u_last->next ? x->u_last->next->name : "no");
        we_are_undoing = 0;
        /* here we call updating of all unpaired hubs and nodes since their
           regular call will fail in case their position needed to be updated
           by undo/redo first to reflect the old one */
        glob_preset_node_list_seek_hub();
        glob_preset_node_list_check_loc_and_update();
        if (glist_isvisible(x) && glist_istoplevel(x))
        {
            gui_vmess("gui_undo_menu", "xss",
                x, undo_action, redo_action);
            text_checkvalidwidth(x);
            canvas_getscroll(x);
        }
        canvas_dirty(x, 1);
    }
    canvas_resume_dsp(dspwas);
}

void canvas_undo_rebranch(t_canvas *x)
{
    int dspwas = canvas_suspend_dsp();
    t_undo_action *a1, *a2;
    //fprintf(stderr,"canvas_undo_rebranch");
    if (x->u_last->next)
    {
        a1 = x->u_last->next;
        while(a1)
        {
            //fprintf(stderr,".");
            canvas_undo_doit(x, a1, UNDO_FREE);
            a2 = a1->next;
            freebytes(a1, sizeof(*a1));
            a1 = a2;
        }
        //x->u_last->next = 0; /* ??? */
    }
    //gui_vmess("gui_undo_menu", "xss", x, x->u_last->name, "no"); /* ??? */
    canvas_resume_dsp(dspwas);
    //fprintf(stderr,"done!\n");
}

void canvas_undo_check_canvas_pointers(t_canvas *x)
{
    /* currently unnecessary unless we decide to implement one central undo
       for all patchers */
}

void canvas_undo_purge_abstraction_actions(t_canvas *x)
{
    /* currently unnecessary unless we decide to implement one central undo
       for all patchers */
}

void canvas_undo_free(t_canvas *x)
{
    int dspwas = canvas_suspend_dsp();
    t_undo_action *a1, *a2;
    //fprintf(stderr,"canvas_undo_free");
    if (x->u_queue)
    {
        a1 = x->u_queue;
        while(a1)
        {
            //fprintf(stderr,".");
            canvas_undo_doit(x, a1, UNDO_FREE);
            a2 = a1->next;
            freebytes(a1, sizeof(*a1));
            a1 = a2;
        }
    }
    canvas_resume_dsp(dspwas);
    //fprintf(stderr,"done!\n");
}

