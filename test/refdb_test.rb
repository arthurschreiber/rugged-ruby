require_relative "test_helper"

require 'tmpdir'

class RefdbBackendTest < MiniTest::Unit::TestCase
  def setup
    @path = Dir.mktmpdir("rugged-empty")
    @repo = Rugged::Repository.init_at(@path)
    @backend = @repo.refdb.backend = Rugged::Refdb::Backend::Ruby.new(@repo)
  end

  def teardown
    FileUtils.remove_entry_secure(@path)
  end

  def test_compress
    compress_calls = 0
    @backend.send(:define_singleton_method, :compress) do
      compress_calls += 1
    end

    @repo.refdb.compress

    assert_equal 1, compress_calls
  end

  def test_compress_exception
    @backend.send(:define_singleton_method, :compress) do
      raise "Fail!"
    end

    err = assert_raises(Rugged::InvalidError) {
      @repo.refdb.compress
    }

    assert_equal "compress: Fail! (RuntimeError)\n", err.message
  end

  def test_lookup
    @backend.send(:define_singleton_method, :lookup) do |ref_name|
      "1385f264afb75a56a5bec74243be9b367ba4ca08" if ref_name == "refs/heads/master"
    end

    ref = @repo.references["refs/heads/master"]
    assert ref
    assert_equal "refs/heads/master", ref.name
    assert_equal "1385f264afb75a56a5bec74243be9b367ba4ca08", ref.target_id

    assert_nil @repo.references["refs/heads/development"]
  end

  def test_lookup_result_no_oid_string
    @backend.send(:define_singleton_method, :lookup) do |ref_name|
      "123" if ref_name == "refs/heads/master"
    end

    err = assert_raises(Rugged::InvalidError) { @repo.references["refs/heads/master"] }
    assert_equal "Unable to parse OID - contains invalid characters", err.message
  end

  def test_lookup_result_no_string
    @backend.send(:define_singleton_method, :lookup) do |ref_name|
      1234 if ref_name == "refs/heads/master"
    end

    err = assert_raises(Rugged::InvalidError) { @repo.references["refs/heads/master"] }
    assert_equal "lookup: wrong argument type Fixnum (expected String) (TypeError)\n", err.message
  end
end
