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

pub type PlainText = [u16; MSG_SHORT_SIZE];

pub const MSG_BYTE_SIZE: usize = 64;
pub const MSG_SHORT_SIZE: usize = MSG_BYTE_SIZE / 2;
pub const CT_BYTE_SIZE: usize = 1276;

#[repr(C)]
#[derive(Debug,Clone)]
pub struct CCiphertext {
    pub(crate) ptr: *mut c_char,
}

impl CCiphertext {
    pub fn encrypt(cek: &CEncryptionKey, plaintext: PlainText) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let msg_vec = plaintext
            .map(|x| x as u64)
            .into_iter()
            .collect::<Vec<u64>>();
        let msg_int = pack(&msg_vec, 64);
        assert_eq!(unpack::<u64>(msg_int.clone(), 64, MSG_SHORT_SIZE), msg_vec);
        let c = Paillier::encrypt(&ek, RawPlaintext::from(msg_int));
        let c_str = Self::rust_type2string(&c); //serde_json::to_string(&c).expect("Fail to serialize a ciphertext");
        let ptr = str2ptr(c_str);
        Self { ptr }
    }

    pub fn decrypt(&self, cdk: &CDecryptionKey) -> PlainText {
        let dk: DecryptionKey = cdk.to_rust_type();
        let c = self.to_rust_type();
        let msg_int = Paillier::decrypt(&dk, c);
        let msg_vecs: Vec<u64> = unpack(msg_int.into(), 64, MSG_SHORT_SIZE);

        let mut plaintext = [0; MSG_SHORT_SIZE];
        for i in 0..MSG_SHORT_SIZE {
            plaintext[i] = msg_vecs[i] as u16;
        }
        plaintext
    }

    pub fn to_rust_type(&self) -> RawCiphertext {
        let c_str: &str = ptr2str(self.ptr);
        /*let c_int = BigInt::from_str_radix(c_vec.to_str().unwrap(), 16).unwrap();
        RawCiphertext::from(c_int)*/

        let c_int: BigInt = BigInt::from_str_radix(c_str, 16).unwrap();
        RawCiphertext::from(c_int)
    }

    pub fn add_ciphertext(&self, other: &Self, cek: &CEncryptionKey) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let c1 = self.to_rust_type();
        let c2 = other.to_rust_type();
        let new_c = Paillier::add(&ek, c1, c2);
        let new_c_str = Self::rust_type2string(&new_c);
        let ptr = str2ptr(new_c_str);
        CCiphertext { ptr }
    }

    pub fn sub_ciphertext(&self, other: &Self, cek: &CEncryptionKey) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let n = ek.n;
        let minus_one = n - BigInt::one();
        let minus_c2 = other.scalar_ciphertext_bigint(minus_one, cek);
        self.add_ciphertext(&minus_c2, cek)
    }

    pub fn scalar_ciphertext(&self, scalar: u16, cek: &CEncryptionKey) -> CCiphertext {
        let scalar = BigInt::from(scalar as u64);
        self.scalar_ciphertext_bigint(scalar, cek)
    }

    pub fn scalar_ciphertext_percent(&self, scalar: u16, cek: &CEncryptionKey) -> CCiphertext {
        let inv100 = get_inv_for_ek(cek, 100);
        let scalar_int = BigInt::from(scalar as u64);
        let ek: EncryptionKey = cek.to_rust_type();
        let n = ek.n.clone();
        let new_scalar_int = (scalar_int * inv100) % n;
        self.scalar_ciphertext_bigint(new_scalar_int, cek)
    }

    fn scalar_ciphertext_bigint(&self, scalar: BigInt, cek: &CEncryptionKey) -> CCiphertext {
        let ek: EncryptionKey = cek.to_rust_type();
        let c = self.to_rust_type();
        let new_c = Paillier::mul(&ek, c, RawPlaintext::from(scalar));
        let new_c_str = Self::rust_type2string(&new_c);
        let ptr = str2ptr(new_c_str);
        CCiphertext { ptr }
    }

    pub(crate) fn rust_type2string(ct: &RawCiphertext) -> String {
        ct.0.to_str_radix(16)
    }

    pub fn drop(&self) {
        drop_ptr(self.ptr)
    }
}
