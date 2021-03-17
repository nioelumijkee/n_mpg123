//----------------------------------------------------------------------------//
int pd_open_array(t_symbol *s_arr, // name
		  t_word **w_arr,  // word
		  t_garray **g_arr) // garray
{
  int len;
  t_word *i_w_arr;
  t_garray *i_g_arr;
  if (!(i_g_arr = (t_garray *)pd_findbyclass(s_arr, garray_class)))
    {
      error("%s: no such array", s_arr->s_name);
      len = -1;
    }
  else if (!garray_getfloatwords(i_g_arr, &len, &i_w_arr))
    {
      error("%s: bad template", s_arr->s_name);
      len = -1;
    }
  else
    {
      *w_arr = i_w_arr;
      *g_arr = i_g_arr;
    }
  return (len);
}
