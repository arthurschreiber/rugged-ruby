#include <ruby.h>
#include <git2.h>
#include <git2/sys/refdb_backend.h>
#include <git2/sys/refs.h>

VALUE rb_mRugged;
VALUE rb_cRuggedRefdb;
VALUE rb_cRuggedRefdbBackend;
VALUE rb_cRuggedRefdbBackendRuby;

typedef struct {
	git_refdb_backend parent;
	VALUE self;
} refdb_ruby_backend;

// Callback to be used with `rb_rescue`
static VALUE rescue_callback(VALUE method)
{
	VALUE error;
	VALUE e = rb_errinfo();
	VALUE msg = rb_funcall(e, rb_intern("message"), 0);
	error = rb_sprintf("%"PRIsVALUE": %"PRIsVALUE" (%s)\n", method, msg, rb_obj_classname(e));
	giterr_set_str(GITERR_INVALID, StringValueCStr(error));
	return Qundef;
}

static VALUE refdb_ruby_backend__exists_call(VALUE *args)
{
	return rb_funcall(args[0], rb_intern("exists"), 1, args[1]);
}

static int refdb_ruby_backend__exists(
	int *exists,
	git_refdb_backend *_backend,
	const char *ref_name)
{
	refdb_ruby_backend *backend = (refdb_ruby_backend *)_backend;
	VALUE args[2], rb_result;

	args[0] = backend->self;
	args[1] = rb_str_new2(ref_name);

	rb_result = rb_rescue(refdb_ruby_backend__exists_call, (VALUE)args, rescue_callback, ID2SYM(rb_intern("exists")));
	if (rb_result == Qundef) {
		return GIT_ERROR;
	}

	*exists = RTEST(rb_result);
	return GIT_OK;
}

static VALUE refdb_ruby_backend__lookup_call(VALUE *args)
{
	VALUE rb_result = rb_funcall(args[0], rb_intern("lookup"), 1, args[1]);
	if (!NIL_P(rb_result))
		Check_Type(rb_result, T_STRING);
	return rb_result;
}

static int refdb_ruby_backend__lookup(
	git_reference **out,
	git_refdb_backend *_backend,
	const char *ref_name)
{
	refdb_ruby_backend *backend = (refdb_ruby_backend *)_backend;
	VALUE args[2], rb_result;

	git_oid oid;
	int error;

	args[0] = backend->self;
	args[1] = rb_str_new2(ref_name);

	rb_result = rb_rescue(refdb_ruby_backend__lookup_call, (VALUE)args, rescue_callback, ID2SYM(rb_intern("lookup")));

	if (rb_result == Qundef)
		return GIT_ERROR;
	else if (rb_result == Qnil)
		return GIT_ENOTFOUND;

	if (!(error = git_oid_fromstr(&oid, StringValueCStr(rb_result)))) {
		*out = git_reference__alloc(ref_name, &oid, NULL);
		if (!*out)
			error = -1;
	}

	return error;
}

static VALUE refdb_ruby_backend__compress_call(VALUE rb_backend)
{
	return rb_funcall(rb_backend, rb_intern("compress"), 0);
}

static int refdb_ruby_backend__compress(git_refdb_backend *_backend)
{
	refdb_ruby_backend *backend = (refdb_ruby_backend *)_backend;

	VALUE rb_result = rb_rescue(refdb_ruby_backend__compress_call, backend->self, rescue_callback, ID2SYM(rb_intern("compress")));

	return (rb_result == Qundef) ? GIT_ERROR : GIT_OK;
}

static void refdb_ruby_backend__free(git_refdb_backend *backend)
{
	if (backend) backend->free(backend);
}

static VALUE rb_git_refdb_ruby_backend_new(VALUE self, VALUE rb_repo)
{
	refdb_ruby_backend *backend;

	backend = xcalloc(1, sizeof(refdb_ruby_backend));

	backend->parent.exists = &refdb_ruby_backend__exists;
	backend->parent.lookup = &refdb_ruby_backend__lookup;
	// backend->parent.iterator = &refdb_ruby_backend__iterator;
	// backend->parent.write = &refdb_ruby_backend__write;
	// backend->parent.del = &refdb_ruby_backend__delete;
	// backend->parent.rename = &refdb_ruby_backend__rename;
	backend->parent.compress = &refdb_ruby_backend__compress;
	// backend->parent.lock = &refdb_ruby_backend__lock;
	// backend->parent.unlock = &refdb_ruby_backend__unlock;
	// backend->parent.has_log = &refdb_ruby_backend__reflog_has_log;
	// backend->parent.ensure_log = &refdb_ruby_backend__reflog_ensure_log;
	// backend->parent.reflog_read = &refdb_ruby_backend__reflog_read;
	// backend->parent.reflog_write = &refdb_ruby_backend__reflog_write;
	// backend->parent.reflog_rename = &refdb_ruby_backend__reflog_rename;
	// backend->parent.reflog_delete = &refdb_ruby_backend__reflog_delete;
	backend->parent.free = (void (*)(git_refdb_backend *)) &xfree;

	backend->self = Data_Wrap_Struct(self, NULL, refdb_ruby_backend__free, backend);
	return backend->self;
}

void Init_rugged_ruby(void)
{
	rb_mRugged = rb_const_get(rb_cObject, rb_intern("Rugged"));
	rb_cRuggedRefdb = rb_const_get(rb_mRugged, rb_intern("Refdb"));
	rb_cRuggedRefdbBackend = rb_const_get(rb_cRuggedRefdb, rb_intern("Backend"));
	rb_cRuggedRefdbBackendRuby = rb_define_class_under(rb_cRuggedRefdbBackend, "Ruby", rb_cRuggedRefdbBackend);

	rb_define_singleton_method(rb_cRuggedRefdbBackendRuby, "new", rb_git_refdb_ruby_backend_new, 1);
}
