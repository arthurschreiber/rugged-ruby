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
} rugged_refdb_backend_custom;

static int rugged_refdb_backend_custom__exists(
	int *exists,
	git_refdb_backend *_backend,
	const char *ref_name)
{
	rugged_refdb_backend_custom *backend = (rugged_refdb_backend_custom *)_backend;

	// TODO: Proper exception handling
	*exists = RTEST(rb_funcall(backend->self, rb_intern("exists"), 1, rb_str_new2(ref_name)));

	return GIT_OK;
}

static int rugged_refdb_backend_custom__lookup(
	git_reference **out,
	git_refdb_backend *_backend,
	const char *ref_name)
{
	rugged_refdb_backend_custom *backend = (rugged_refdb_backend_custom *)_backend;
	git_oid oid;
	VALUE rb_result;

	// TODO: Proper exception handling
	rb_result = rb_funcall(backend->self, rb_intern("lookup"), 1, rb_str_new2(ref_name));
	if (TYPE(rb_result) == T_STRING) {
		// TODO: Handle symbolic refs as well
		// TODO: Proper exception handling
		git_oid_fromstr(&oid, StringValueCStr(rb_result));
		*out = git_reference__alloc(ref_name, &oid, NULL);
		return GIT_OK;
	} else {
		// TODO: Better error handling
		*out = NULL;
		return GIT_ENOTFOUND;
	}
}

static int rugged_refdb_backend_custom__compress(git_refdb_backend *_backend)
{
	rugged_refdb_backend_custom *backend = (rugged_refdb_backend_custom *)_backend;

	// TODO: Proper exception handling
	rb_funcall(backend->self, rb_intern("compress"), 0);

	return GIT_OK;
}

static void rb_git_refdb_backend__free(git_refdb_backend *backend)
{
	if (backend) backend->free(backend);
}

static VALUE rb_git_refdb_backend_custom_new(VALUE self, VALUE rb_repo)
{
	rugged_refdb_backend_custom *backend;

	backend = xcalloc(1, sizeof(rugged_refdb_backend_custom));

	backend->parent.exists = &rugged_refdb_backend_custom__exists;
	backend->parent.compress = &rugged_refdb_backend_custom__compress;
	backend->parent.lookup = &rugged_refdb_backend_custom__lookup;
	backend->parent.free = (void (*)(git_refdb_backend *)) &xfree;

	backend->self = Data_Wrap_Struct(self, NULL, rb_git_refdb_backend__free, backend);
	return backend->self;
}

void Init_rugged_ruby(void)
{
	rb_mRugged = rb_const_get(rb_cObject, rb_intern("Rugged"));
	rb_cRuggedRefdb = rb_const_get(rb_mRugged, rb_intern("Refdb"));
	rb_cRuggedRefdbBackend = rb_const_get(rb_cRuggedRefdb, rb_intern("Backend"));
	rb_cRuggedRefdbBackendRuby = rb_define_class_under(rb_cRuggedRefdbBackend, "Ruby", rb_cRuggedRefdbBackend);

	rb_define_singleton_method(rb_cRuggedRefdbBackendRuby, "new", rb_git_refdb_backend_custom_new, 1);
}
