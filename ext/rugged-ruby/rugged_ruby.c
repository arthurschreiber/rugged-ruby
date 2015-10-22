#include <ruby.h>
#include <git2.h>
#include <git2/sys/config.h>
#include <git2/sys/refdb_backend.h>
#include <git2/sys/refs.h>

VALUE rb_mRugged;
VALUE rb_cRuggedRefdb;
VALUE rb_cRuggedRefdbBackend;
VALUE rb_cRuggedRefdbBackendRuby;
VALUE rb_cRuggedConfig;
VALUE rb_cRuggedConfigBackend;
VALUE rb_cRuggedConfigBackendRuby;

typedef struct {
	git_refdb_backend parent;
	VALUE self;
} refdb_ruby_backend;

typedef struct {
	struct git_config_backend parent;
	VALUE self;
} config_ruby_backend;

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

static VALUE refdb_ruby_backend__write_call(VALUE *args)
{
	return rb_funcall(args[0], rb_intern("write"), 6, args[1], args[2], args[3], args[4], args[5], args[6]);
}

static int refdb_ruby_backend__write(
	git_refdb_backend *_backend,
	const git_reference *ref,
	int force,
	const git_signature *who,
	const char *message,
	const git_oid *old_id,
	const char *old_target)
{
	refdb_ruby_backend *backend = (refdb_ruby_backend *)_backend;
	VALUE rb_result;
	VALUE args[7];

	args[0] = backend->self;

	rb_result = rb_rescue(refdb_ruby_backend__write_call, (VALUE) args, rescue_callback, ID2SYM(rb_intern("write")));

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

void validate_config_level_symbol(VALUE rb_level)
{
	ID id_level;
	Check_Type(rb_level, T_SYMBOL);
	id_level = SYM2ID(rb_level);

	if (id_level != rb_intern("system") && id_level != rb_intern("xdg") &&
	    id_level != rb_intern("global") && id_level != rb_intern("local") &&
	    id_level != rb_intern("app") && id_level != rb_intern("highest"))
		rb_raise(rb_eArgError, "Invalid config backend level.");
}

git_config_level_t symbol_to_config_level(VALUE rb_level)
{
	ID id_level = SYM2ID(rb_level);

	if (id_level == rb_intern("system")) {
		return GIT_CONFIG_LEVEL_SYSTEM;
	} else if (id_level == rb_intern("xdg")) {
		return GIT_CONFIG_LEVEL_XDG;
	} else if (id_level == rb_intern("global")) {
		return GIT_CONFIG_LEVEL_GLOBAL;
	} else if (id_level == rb_intern("local")) {
		return GIT_CONFIG_LEVEL_LOCAL;
	} else if (id_level == rb_intern("app")) {
		return GIT_CONFIG_LEVEL_APP;
	} else if (id_level == rb_intern("highest")) {
		return GIT_CONFIG_HIGHEST_LEVEL;
	} else {
		rb_raise(rb_eArgError, "Invalid config backend level.");
	}
}

static VALUE config_level_to_symbol(git_config_level_t level)
{
	if (level == GIT_CONFIG_LEVEL_SYSTEM) {
		return ID2SYM(rb_intern("system"));
	} else if (level == GIT_CONFIG_LEVEL_XDG) {
		return ID2SYM(rb_intern("xdg"));
	} else if (level == GIT_CONFIG_LEVEL_GLOBAL) {
		return ID2SYM(rb_intern("global"));
	} else if (level == GIT_CONFIG_LEVEL_LOCAL) {
		return ID2SYM(rb_intern("local"));
	} else if (level == GIT_CONFIG_LEVEL_APP) {
		return ID2SYM(rb_intern("app"));
	} else if (level == GIT_CONFIG_HIGHEST_LEVEL) {
		return ID2SYM(rb_intern("highest"));
	} else {
		// This should never happen
		return Qundef;
	}
}

static VALUE config_ruby_backend__get_call(VALUE *args)
{
	VALUE rb_result = rb_funcall(args[0], rb_intern("get"), 1, args[1]);

	if (NIL_P(rb_result))
		return rb_result;

	Check_Type(rb_result, T_HASH);
	Check_Type(rb_hash_aref(rb_result, ID2SYM(rb_intern("name"))), T_STRING);
	Check_Type(rb_hash_aref(rb_result, ID2SYM(rb_intern("value"))), T_STRING);
	validate_config_level_symbol(rb_hash_aref(rb_result, ID2SYM(rb_intern("level"))));

	return rb_result;
}

int config_ruby_backend__get(struct git_config_backend *_backend, const char *key, git_config_entry **entry)
{
	VALUE rb_result, args[2], rb_name, rb_value;
	config_ruby_backend *backend = (config_ruby_backend *)_backend;

	args[0] = backend->self;
	args[1] = rb_str_new2(key);

	rb_result = rb_rescue(config_ruby_backend__get_call, (VALUE) args, rescue_callback, ID2SYM(rb_intern("get")));

	if (rb_result == Qundef)
		return GIT_ERROR;
	else if (NIL_P(rb_result))
		return GIT_ENOTFOUND;

	*entry = xcalloc(1, sizeof(git_config_entry));
	rb_name = rb_hash_aref(rb_result, ID2SYM(rb_intern("name")));
	(*entry)->name = StringValueCStr(rb_name);
	rb_value = rb_hash_aref(rb_result, ID2SYM(rb_intern("value")));
	(*entry)->value = StringValueCStr(rb_value);
	(*entry)->level = symbol_to_config_level(rb_hash_aref(rb_result, ID2SYM(rb_intern("level"))));
	(*entry)->free = (void (*)(git_config_entry *)) &xfree;

	return GIT_OK;
}

static VALUE config_ruby_backend__set_call(VALUE *args)
{
	return rb_funcall(args[0], rb_intern("set"), 2, args[1], args[2]);
}

int config_ruby_backend__set(struct git_config_backend *_backend, const char *key, const char *value)
{
	VALUE rb_result, args[3];
	config_ruby_backend *backend = (config_ruby_backend *)_backend;

	args[0] = backend->self;
	args[1] = rb_str_new2(key);
	args[2] = rb_str_new2(value);

	rb_result = rb_rescue(config_ruby_backend__set_call, (VALUE) args, rescue_callback, ID2SYM(rb_intern("set")));

	return (rb_result == Qundef) ? GIT_ERROR : GIT_OK;
}

static VALUE config_ruby_backend__open_call(VALUE *args)
{
	return rb_funcall(args[0], rb_intern("open"), 1, args[1]);
}

int config_ruby_backend__open(struct git_config_backend *_backend, git_config_level_t level)
{
	VALUE rb_result, args[2];
	config_ruby_backend *backend = (config_ruby_backend *)_backend;

	args[0] = backend->self;
	args[1] = config_level_to_symbol(level);

	rb_result = rb_rescue(config_ruby_backend__open_call, (VALUE) args, rescue_callback, ID2SYM(rb_intern("open")));

	return (rb_result == Qundef) ? GIT_ERROR : GIT_OK;
}

static void config_ruby_backend__free(struct git_config_backend *backend)
{
		if (backend) backend->free(backend);
}

static VALUE rb_git_config_ruby_backend_new(VALUE self)
{
	config_ruby_backend *backend;

	backend = xcalloc(1, sizeof(config_ruby_backend));
	backend->parent.version = GIT_CONFIG_BACKEND_VERSION;

	backend->parent.open = &config_ruby_backend__open;
	backend->parent.get = &config_ruby_backend__get;
	backend->parent.set = &config_ruby_backend__set;
//	backend->parent.set_multivar = &config_ruby_backend__set_multivar;
//	backend->parent.del = &config_ruby_backend__del;
//	backend->parent.del_multivar = &config_ruby_backend__del_multivar;
//	backend->parent.iterator = &config_ruby_backend__iterator;
//	backend->parent.snapshot = &config_ruby_backend__snapshot;
//	backend->parent.lock = &config_ruby_backend__lock;
//	backend->parent.unlock = &config_ruby_backend__unlock;
	backend->parent.free = (void (*)(struct git_config_backend *)) &xfree;

	backend->self = Data_Wrap_Struct(self, NULL, config_ruby_backend__free, backend);
	return backend->self;
}

void Init_rugged_ruby(void)
{
	rb_mRugged = rb_const_get(rb_cObject, rb_intern("Rugged"));
	rb_cRuggedRefdb = rb_const_get(rb_mRugged, rb_intern("Refdb"));
	rb_cRuggedRefdbBackend = rb_const_get(rb_cRuggedRefdb, rb_intern("Backend"));
	rb_cRuggedRefdbBackendRuby = rb_define_class_under(rb_cRuggedRefdbBackend, "Ruby", rb_cRuggedRefdbBackend);

	rb_cRuggedConfig = rb_const_get(rb_mRugged, rb_intern("Config"));
	rb_cRuggedConfigBackend = rb_const_get(rb_cRuggedConfig, rb_intern("Backend"));
	rb_cRuggedConfigBackendRuby = rb_define_class_under(rb_cRuggedConfigBackend, "Ruby", rb_cRuggedConfigBackend);

	rb_define_singleton_method(rb_cRuggedRefdbBackendRuby, "new", rb_git_refdb_ruby_backend_new, 1);
	rb_define_singleton_method(rb_cRuggedConfigBackendRuby, "new", rb_git_config_ruby_backend_new, 0);
}
