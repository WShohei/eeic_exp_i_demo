curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain nightly &&
source $HOME/.cargo/env &&
cargo install --force cbindgen