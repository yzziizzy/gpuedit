


#define charp char*
#define charpp char**
#define bool bool
#define Color4 Color4
#define tabsp TabSpec*
#define widsp WidgetSpec*
#define scrollfn int

// strvec is an accumulating type, so values found in later config files will be appended instead of overwriting prior values
#ifndef STRVEC_TYTPEDEFFED
	#define STRVEC_TYTPEDEFFED 1
	typedef VEC(char*) strvec;
#endif

#define true 1
#define false 0

#define set_int(x) x;
#define set_bool(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_charp(x) safe_strdup(x);
#define set_charpp(x) strlistdup(x);
#define set_Color4(x) x;
#define set_tabsp(x) tabspeclistdup(x);
#define set_scrollfn(x) x;
#define set_widsp(x) widgetspeclistdup(x);

#define free_int(x)
#define free_bool(x)
#define free_float(x)
#define free_double(x)
#define free_charp(x) if(x) free(x);
#define free_charpp(x) freeptrlist(x);
#define free_tabsp(x) freetabspeclist(x);
#define free_Color4(x)
#define free_scrollfn(x)
#define free_widsp(x) freewidgetspeclist(x);


#define copy_int(x) x;
#define copy_bool(x) x;
#define copy_float(x) x;
#define copy_double(x) x;
#define copy_charp(x) safe_strdup(x);
#define copy_charpp(x) strlistdup(x);
#define copy_Color4(x) x;
#define copy_tabsp(x) tabspeclistdup(x);
#define copy_scrollfn(x) x;
#define copy_widsp(x) widgetspeclistdup(x);


