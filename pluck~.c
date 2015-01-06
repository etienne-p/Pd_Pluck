//#include <stdlib.h>
#include <math.h>
#include "m_pd.h"

#define MAX_BUFFER_SIZE 2048

static t_class *pluck_tilde_class;

typedef struct _pluck_tilde {
    
    t_object x_obj;
    int delay;
    int index;
    float feedback;
    float dry;
    float alpha;
    float * buffer;
    
} t_pluck_tilde;

t_int *pluck_tilde_perform(t_int *w)
{
    t_pluck_tilde *x = (t_pluck_tilde *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    t_int n = (t_int)(w[3]);
    
    int delay = x->delay;
    int index = x->index;
    int prevIndex = x->index % delay;
    float fdry = x->feedback * x->dry;
    float flp = x->feedback * (1.0f - x->dry);
    float alpha = x->alpha;
    
    while(n--){
        index = (prevIndex + 1) % delay;
        x->buffer[index] = fdry * x->buffer[index] + flp * (alpha * x->buffer[index] + (1.0f - alpha) * x->buffer[prevIndex]);
        *out++ = x->buffer[index];
        prevIndex = index;
    }
    
    x->index = index;
    
    return (w+4);
}

void pluck_tilde_freq (t_pluck_tilde *x, t_floatarg freq)
{
    if(freq <= 0) {
        error("pluck: freq should be be >= 0");
        return;
    }
    int delay = sys_getsr() / freq;
    x->delay = delay > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : delay;
}

void pluck_tilde_feedback (t_pluck_tilde *x, t_floatarg feedback)
{
    if (feedback < 0 || feedback > 1){
        error("pluck: feedback should be in [0, 1] interval");
        return;
    }
    x->feedback = feedback;
}

void pluck_tilde_dry (t_pluck_tilde *x, t_floatarg dry)
{
    if (dry < 0 || dry > 1){
        error("pluck: dry should be in [0, 1] interval");
        return;
    }
    x->dry = dry;
}

void pluck_tilde_alpha (t_pluck_tilde *x, t_floatarg alpha)
{
    if (alpha < 0 || alpha > 1){
        error("pluck: alpha should be in [0, 1] interval");
        return;
    }
    x->alpha = alpha;
}

void pluck_tilde_bang(t_pluck_tilde *x)
{
    // fill buffer with noise, see d_osc.c in pd source code for noise generation
    int val = 307 * 1319;
    for( int i = 0 ; i < MAX_BUFFER_SIZE; i++ ) {
        x->buffer[i] = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
        val = val * 435898247 + 382842987;
    }
}

void pluck_tilde_dsp(t_pluck_tilde *x, t_signal **sp)
{
    dsp_add(pluck_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void pluck_tilde_free(t_pluck_tilde *x)
{
    t_freebytes(x->buffer, MAX_BUFFER_SIZE * sizeof(float));
}

void *pluck_tilde_new(t_floatarg f)
{
    t_pluck_tilde *x = (t_pluck_tilde *)pd_new(pluck_tilde_class);
    
    x->alpha = .5f;
    x->feedback = .5f;
    x->dry = .5f;
    x->delay = MAX_BUFFER_SIZE;
    x->buffer = (float *) t_getbytes(MAX_BUFFER_SIZE * sizeof(float));
    
    outlet_new(&x->x_obj, gensym("signal") );
    
    return (void *)x;
}

void pluck_tilde_setup(void) {
    
    pluck_tilde_class = class_new(gensym("pluck~"),
                                  (t_newmethod)pluck_tilde_new,
                                  (t_method)pluck_tilde_free, sizeof(t_pluck_tilde),
                                  CLASS_DEFAULT, A_GIMME,0);
    
    class_addmethod(pluck_tilde_class, (t_method)pluck_tilde_dsp, gensym("dsp"), 0);
    
    class_addmethod(pluck_tilde_class, (t_method)pluck_tilde_freq, gensym("freq"), A_FLOAT, 0);
    class_addmethod(pluck_tilde_class, (t_method)pluck_tilde_feedback, gensym("feedback"), A_FLOAT, 0);
    class_addmethod(pluck_tilde_class, (t_method)pluck_tilde_dry, gensym("dry"), A_FLOAT, 0);
    class_addmethod(pluck_tilde_class, (t_method)pluck_tilde_alpha, gensym("alpha"), A_FLOAT, 0);
    
    class_addbang(pluck_tilde_class,(t_method)pluck_tilde_bang);
}
