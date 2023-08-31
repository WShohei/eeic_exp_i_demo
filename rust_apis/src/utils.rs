use paillier::arithimpl::traits::ConvertFrom;
use paillier::arithimpl::traits::ModInv;
use paillier::arithimpl::traits::ModMul;
use paillier::*;
//use std::ffi::CString;
use crate::CEncryptionKey;
use std::ffi::*;
use std::marker::PhantomData;
use std::mem;
use std::os::raw::c_char;
use std::slice;
use std::str::FromStr;

/// Compute 1/inv mod n
pub(crate) fn get_inv_for_ek(cek: &CEncryptionKey, inv: u16) -> BigInt {
    let ek: EncryptionKey = cek.to_rust_type();
    let n = ek.n;
    let inv = BigInt::from(inv as u64);
    let reversed = BigInt::modinv(&inv, &n);
    assert_eq!(BigInt::modmul(&inv, &reversed, &n), BigInt::one());
    reversed
}

pub(crate) fn str2ptr(str: String) -> *mut c_char {
    let c_str = CString::new(str).unwrap();
    c_str.into_raw()
}

pub(crate) fn ptr2str<'a>(ptr: *mut c_char) -> &'a str {
    let cstr = unsafe { CStr::from_ptr(ptr) };
    cstr.to_str().unwrap()
}

pub(crate) fn drop_ptr(ptr: *mut c_char) {
    let cstring = unsafe {CString::from_raw(ptr)};
    drop(cstring);
}
