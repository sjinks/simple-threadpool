#ifndef F1EA37D2_C370_42BF_8B5D_3BB7A6451A93
#define F1EA37D2_C370_42BF_8B5D_3BB7A6451A93

#ifdef WWA_SIMPLE_THREADPOOL_STATIC_DEFINE
#    define WWA_SIMPLE_THREADPOOL_EXPORT
#    define WWA_SIMPLE_THREADPOOL_NO_EXPORT
#else
#    ifdef wwa_simple_threadpool_EXPORTS
/* We are building this library; export */
#        if defined _WIN32 || defined __CYGWIN__
#            define WWA_SIMPLE_THREADPOOL_EXPORT __declspec(dllexport)
#            define WWA_SIMPLE_THREADPOOL_NO_EXPORT
#        else
#            define WWA_SIMPLE_THREADPOOL_EXPORT    [[gnu::visibility("default")]]
#            define WWA_SIMPLE_THREADPOOL_NO_EXPORT [[gnu::visibility("hidden")]]
#        endif
#    else
/* We are using this library; import */
#        if defined _WIN32 || defined __CYGWIN__
#            define WWA_SIMPLE_THREADPOOL_EXPORT __declspec(dllimport)
#            define WWA_SIMPLE_THREADPOOL_NO_EXPORT
#        else
#            define WWA_SIMPLE_THREADPOOL_EXPORT    [[gnu::visibility("default")]]
#            define WWA_SIMPLE_THREADPOOL_NO_EXPORT [[gnu::visibility("hidden")]]
#        endif
#    endif
#endif

#endif /* F1EA37D2_C370_42BF_8B5D_3BB7A6451A93 */
