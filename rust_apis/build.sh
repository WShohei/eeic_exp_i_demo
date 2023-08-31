cargo +nightly build --release &&
cbindgen --config cbindgen.toml --crate rust_apis --output rust_apis.h &&
cp ./target/release/librust_apis.a ./librust_apis.a