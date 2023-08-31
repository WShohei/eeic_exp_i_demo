use paillier::arithimpl::traits::ConvertFrom;
use paillier::arithimpl::traits::ModInv;
use paillier::arithimpl::traits::ModMul;
use paillier::*;
//use std::ffi::CString;
use std::ffi::*;
use std::marker::PhantomData;
use std::mem;
use std::os::raw::c_char;
use std::slice;
use std::str::FromStr;

use crate::*;

pub const EK_BYTE_SIZE: usize = 625;
pub const DK_BYTE_SIZE: usize = 633;

#[repr(C)]
#[derive(Debug,Clone)]
pub struct CKeys {
    pub ek: CEncryptionKey,
    pub dk: CDecryptionKey,
}

#[repr(C)]
#[derive(Debug,Clone)]
pub struct CEncryptionKey {
    ptr: *mut c_char,
}

#[repr(C)]
#[derive(Debug,Clone)]
pub struct CDecryptionKey {
    ptr: *mut c_char,
}

impl CKeys {
    pub fn new() -> Self {
        let (ek, dk) = Paillier::keypair().keys();
        let ek_ptr =
            str2ptr(serde_json::to_string(&ek).expect("Fail to serialized an encryption key"));
        let dk_ptr =
            str2ptr(serde_json::to_string(&dk).expect("Fail to serialized a decryption key"));
        let ek = CEncryptionKey { ptr: ek_ptr };
        let dk = CDecryptionKey { ptr: dk_ptr };
        CKeys { ek, dk }
    }
}

impl CEncryptionKey {
    pub fn to_rust_type(&self) -> EncryptionKey {
        let ek_vec = ptr2str(self.ptr);
        let ek = serde_json::from_str(&ek_vec).expect("Fail to decode an encryption key bytes");
        ek
    }

    pub fn get_n(&self) -> BigInt {
        self.to_rust_type().n
    }
}

impl CDecryptionKey {
    pub fn to_rust_type(&self) -> DecryptionKey {
        let dk_vec = ptr2str(self.ptr);
        let dk = serde_json::from_str(&dk_vec).expect("Fail to decode a decryption key bytes");
        dk
    }
}
