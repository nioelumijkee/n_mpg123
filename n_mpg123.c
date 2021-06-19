#include <string.h>
#include <mpg123.h>
#include "m_pd.h"
#include "pd_open_array.c"

#define A_ERROR -1
#define OUTLETF(S, F)                                   \
  {                                                     \
    t_atom a[1];                                        \
    SETFLOAT(a, (t_float)(F));                          \
    outlet_anything(x->out, gensym((S)), 1, a);		    \
  }

#define OUTLETS(S, A)                           \
  {                                             \
    t_atom a[1];                                \
    SETSYMBOL(a, gensym((A)));				    \
    outlet_anything(x->out, gensym((S)), 1, a); \
  }

static t_class *n_mpg123_class;

typedef struct _n_mpg123
{
  t_object x_obj;
  t_outlet *out;
  t_word *w[2]; /* array */
  t_garray *g[2];
  int l[2];
  mpg123_handle *mh; /* decoder */
  unsigned char *buffer; /* buffer */
  size_t buffer_size;
  char infile[MAXPDSTRING]; /* file */
  int channels;
  long rate; /* sample rate */
  int len_frame; /* length 1 frame */
  off_t len_in_frames; /* length in frames */
  off_t len_in_samples; /* length in samples */
  t_float len_in_sec; /* length in seconds */
} t_n_mpg123;

//----------------------------------------------------------------------------//
void n_mpg123_free(t_n_mpg123 *x)
{
  free(x->buffer);
  mpg123_close(x->mh);
  mpg123_delete(x->mh);
  mpg123_exit();
}

//----------------------------------------------------------------------------//
void n_mpg123_openfile(t_n_mpg123 *x, t_symbol *file)
{
  int encoding = 0;
  int err = MPG123_OK;

  x->mh = NULL;
  x->buffer = NULL;
  x->buffer_size = 0;
  x->infile[0] = '\0';
  x->channels = 0;
  x->rate = 0;
  x->len_frame = 0;
  x->len_in_frames = 0;
  x->len_in_samples = 0;
  x->len_in_sec = 0;
  
  // free
  mpg123_close(x->mh);
  mpg123_delete(x->mh);
  mpg123_exit();
  
  // filename
  strcpy(x->infile, file->s_name);
  
  // init
  err = mpg123_init();
  if (err != MPG123_OK)
    {
      error("Basic setup goes wrong: %s", mpg123_plain_strerror(err));
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
  
  // new
  x->mh = mpg123_new(NULL, &err);
  if (err != MPG123_OK || x->mh == NULL)
    {
      error("Basic setup goes wrong: %s", mpg123_plain_strerror(err));
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
  
  // let mpg123 work with the file
  err = mpg123_open(x->mh, x->infile);
  if (err != MPG123_OK)
    {
      error("Trouble with mpg123: %s\n", mpg123_strerror(x->mh));
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
  
  // getformat
  if (mpg123_getformat(x->mh, &x->rate, &x->channels, &encoding) != MPG123_OK)
    {
      error("Trouble with mpg123: %s\n", mpg123_strerror(x->mh));
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
  
  // ensure that this output format will not change (it might, when we allow it).
  mpg123_format_none(x->mh);
  mpg123_format(x->mh, x->rate, x->channels, encoding);
  
  // buffer could be almost any size here
  // mpg123_outblock() is just some recommendation.
  // the size should be a multiple of the PCM frame size.
  x->buffer_size = mpg123_outblock(x->mh);
  x->buffer = malloc(x->buffer_size);
  
  // length frame in samples
  x->len_frame = mpg123_spf(x->mh);
 
  // length in frames
  x->len_in_frames = mpg123_framelength(x->mh);
  if (x->len_in_frames == MPG123_ERR)
    {
      error("Trouble with mpg123: %s\n", mpg123_strerror(x->mh));
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
    
 
  // length file in samples
  x->len_in_samples = mpg123_length(x->mh);
  if (x->len_in_samples == MPG123_ERR)
    {
      error("mpg123_length");
      n_mpg123_free(x);
      OUTLETF("error", A_ERROR);
      return;
    }
  
  // length file in seconds
  x->len_in_sec = (t_float)x->len_in_samples / (t_float)x->rate; 
  
  // out
  OUTLETS("filename",        x->infile);
  OUTLETF("channels",        x->channels);
  OUTLETF("rate",            x->rate);
  OUTLETF("buffer size",     x->buffer_size);
  OUTLETF("len in frames",   x->len_in_frames);
  OUTLETF("len in samples",  x->len_in_samples);
  OUTLETF("len in seconds",  x->len_in_sec);
}

//----------------------------------------------------------------------------//
void n_mpg123_open_array(t_n_mpg123 *x, t_symbol *s0, t_symbol *s1)
{
  if (x->channels == 1)
    {
      x->l[0] = pd_open_array(s0, &x->w[0], &x->g[0]);
      if (x->l[0] < 1)
        return;
      garray_resize(x->g[0], (t_float)x->len_in_samples);
      x->l[0] = pd_open_array(s0, &x->w[0], &x->g[0]);
    }
  else if (x->channels == 2)
    {
      x->l[0] = pd_open_array(s0, &x->w[0], &x->g[0]);
      if (x->l[0] < 1)
        return;
      garray_resize(x->g[0], (t_float)x->len_in_samples);
      x->l[0] = pd_open_array(s0, &x->w[0], &x->g[0]);
      
      x->l[1] = pd_open_array(s1, &x->w[1], &x->g[1]);
      if (x->l[1] < 1)
        return;
      garray_resize(x->g[1], (t_float)x->len_in_samples);
      x->l[1] = pd_open_array(s1, &x->w[1], &x->g[1]);
    }
  OUTLETF("array_length", x->l[0]);
}

//----------------------------------------------------------------------------//
void n_mpg123_redraw(t_n_mpg123 *x)
{
  if (x->channels == 1)
    {
      garray_redraw(x->g[0]);
    }
  else if (x->channels == 2)
    {
      garray_redraw(x->g[0]);
      garray_redraw(x->g[1]);
    }
}

//----------------------------------------------------------------------------//
void n_mpg123_decode(t_n_mpg123 *x)
{
  size_t done;
  int err;
  int buf_l;
  int buf_r;
  char ch;
  int i, j, k, fr;
  int count;
  
  // --------------------------------------------------------------------- //
  fr = 0;
  count = 0;
  done = 1;
  err = MPG123_OK;
  do {
    err = mpg123_read(x->mh, x->buffer, x->buffer_size, &done);
    
    // mono
    if (x->channels == 1)
      {
        k = done >> 1; // 2 byte
        for (i = 0; i < k; i++)
          {
            j = i << 1;
            
            ch = x->buffer[j + 1];
            buf_l = ch;
            buf_l = buf_l << 8;
            buf_l += x->buffer[j];
            
            x->w[0][count].w_float = buf_l / 32768.;
            count++;
            if (count >= x->l[0])
              count = x->l[0] - 1;
          }
      }
    
    // stereo
    else if (x->channels == 2)
      {
        k = done >> 2; // 2 byte * 2
        for (i = 0; i < k; i++)
          {
            j = i << 2;
            
            ch = x->buffer[j + 1];
            buf_l = ch;
            buf_l = buf_l << 8;
            buf_l += x->buffer[j];
            
            ch = x->buffer[j + 3];
            buf_r = ch;
            buf_r = buf_r << 8;
            buf_r += x->buffer[j + 2];
            
            x->w[0][count].w_float = buf_l / 32768.;
            x->w[1][count].w_float = buf_r / 32768.;
            count++;
            if (count >= x->l[0])
              count = x->l[0] - 1;
          }
      }
    fr++;
  }   while (done && err == MPG123_OK);
  
  
  // end
  OUTLETF("decoded_frames", fr);
}

//----------------------------------------------------------------------------//
static void *n_mpg123_new(void)
{
  t_n_mpg123 *x = (t_n_mpg123 *)pd_new(n_mpg123_class);
  x->out = outlet_new(&x->x_obj, 0);
  return (x);
}

//----------------------------------------------------------------------------//
void n_mpg123_setup(void)
{
  n_mpg123_class = class_new(gensym("n_mpg123"), (t_newmethod)n_mpg123_new, (t_method)n_mpg123_free, sizeof(t_n_mpg123), 0, A_GIMME, 0);
  class_addmethod(n_mpg123_class, (t_method)n_mpg123_openfile, gensym("openfile"), A_SYMBOL, 0);
  class_addmethod(n_mpg123_class, (t_method)n_mpg123_decode, gensym("decode"), 0);
  class_addmethod(n_mpg123_class, (t_method)n_mpg123_open_array, gensym("array"), A_SYMBOL, A_SYMBOL, 0);
  class_addmethod(n_mpg123_class, (t_method)n_mpg123_redraw, gensym("redraw"), 0);
}
