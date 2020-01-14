#pragma once

#pragma region macros

#ifndef __MACROS__
#define __MACROS__

template <typename F> struct _scope_exit_t {
	_scope_exit_t(F f) : f(f) {}
	~_scope_exit_t() { f(); }
	F f;
};
template <typename F> _scope_exit_t<F> _make_scope_exit_t(F f) {
	return _scope_exit_t<F>(f);
};
#define _concat_impl(arg1, arg2) arg1 ## arg2
#define _concat(arg1, arg2) _concat_impl(arg1, arg2)
#define defer(code) \
	auto _concat(scope_exit_, __COUNTER__) = _make_scope_exit_t([=](){ code; })

//replaces glfunction() with gl(function()) and prints errors automatically
#ifdef NDEBUG
#define gl(OPENGL_CALL) \
	gl##OPENGL_CALL
#else
#define gl(OPENGL_CALL) \
	gl##OPENGL_CALL; \
	{ \
		int potentialGLError = glGetError(); \
		if (potentialGLError != GL_NO_ERROR) \
		{\
			assert(potentialGLError == GL_NO_ERROR && "gl" #OPENGL_CALL); \
		}\
	}
#endif
#endif

//fprintf(stderr, "OpenGL Error '%s' (%d) gl" #OPENGL_CALL " " __FILE__ ":%d\n", (const char*)gluErrorString(potentialGLError), potentialGLError, __LINE__); \