use paillier::arithimpl::traits::ConvertFrom;
use paillier::arithimpl::traits::ModInv;
use paillier::arithimpl::traits::ModMul;
use paillier::*;
//use std::ffi::CString;
use rayon::prelude::*;
use std::ffi::*;
use std::marker::PhantomData;
use std::mem;
use std::os::raw::c_char;
use std::slice;
use std::str::FromStr;
use crossbeam;
use std::io::{stdout, Write, BufWriter};

use crate::*;
pub const NUM_CHUNK: usize = 32;

#[derive(Debug,Clone)]
pub(crate) struct BatchedCT {
    cts: Vec<CCiphertext>,
}

impl BatchedCT {
    /*pub(crate) const MSG_BYTE_SIZE: usize = CCiphertext::MSG_BYTE_SIZE * NUM_CHUNK;
    pub(crate) const MSG_SHORT_SIZE: usize = MSG_BYTE_SIZE / 2;
    pub(crate) const CT_BYTE_SIZE: usize = CCiphertext::CT_BYTE_SIZE * NUM_CHUNK;*/

    pub fn as_ptr(self) -> *const CCiphertext {
        let ptr = self.cts.as_ptr();
        mem::forget(self.cts);
        ptr
    }

    pub fn from_ptr(ptr: *const CCiphertext) -> Self {
        let slice = unsafe { slice::from_raw_parts(ptr, NUM_CHUNK) };
        let cts = slice.to_vec();
        Self {cts}
    }

    pub fn encrypt(cek: &CEncryptionKey, plaintexts: &[PlainText]) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let c_strs = plaintexts
            .par_iter()
            .map(|plaintext| {
                let msg_vec = plaintext
                    .map(|x| x as u64)
                    .into_iter()
                    .collect::<Vec<u64>>();
                let msg_int = pack(&msg_vec, 64);
                //assert_eq!(unpack::<u64>(msg_int.clone(), 64, MSG_SHORT_SIZE), msg_vec);
                let c = Paillier::encrypt(&ek, RawPlaintext::from(msg_int));
                let c_str = CCiphertext::rust_type2string(&c);
                c_str
            })
            .collect::<Vec<String>>();
        let cts = c_strs
            .into_iter()
            .map(|c_str| {
                let ptr = str2ptr(c_str);
                CCiphertext { ptr }
            })
            .collect::<Vec<CCiphertext>>();
        Self { cts }
    }

    pub fn decrypt(&self, cdk: &CDecryptionKey) -> Vec<PlainText> {
        let dk: DecryptionKey = cdk.to_rust_type();
        let cs = self.to_rust_type();
        cs.par_iter()
            .map(|c| {
                let msg_int = Paillier::decrypt(&dk, c);
                let msg_vecs: Vec<u64> = unpack(msg_int.into(), 64, MSG_SHORT_SIZE);
                let mut plaintext = [0; MSG_SHORT_SIZE];
                for i in 0..MSG_SHORT_SIZE {
                    plaintext[i] = msg_vecs[i] as u16;
                }
                plaintext
            })
            .collect()
    }

    /*pub fn decrypt_and_print(self, cdk: &CDecryptionKey) {
        let dk: DecryptionKey = cdk.to_rust_type();
        let cs = &self.to_rust_type();
        crossbeam::scope(|scope| {
            scope.spawn(|_| {
                let decrypted = cs.par_iter()
                .map(|c| {
                    let msg_int = Paillier::decrypt(&dk, c);
                    let msg_vecs: Vec<u64> = unpack(msg_int.into(), 64, MSG_SHORT_SIZE);
                    let mut plaintext = [0; MSG_SHORT_SIZE];
                    for i in 0..MSG_SHORT_SIZE {
                        plaintext[i] = msg_vecs[i] as u16;
                    }
                    plaintext
                })
                .flatten()
                .collect::<Vec<u16>>();
                let out = stdout();
                let mut out = BufWriter::new(out.lock());
                let mut s = String::with_capacity(4096);
                for p in decrypted.iter() {
                    s += &p.to_string();
                }
                out.write_all(s.as_bytes()).unwrap();
            }); 
        }).unwrap();
    }*/

    pub fn to_rust_type(&self) -> Vec<RawCiphertext> {
        self.cts.iter().map(|ct| ct.to_rust_type()).collect()
    }

    pub fn add_ciphertext(&self, other: &Self, cek: &CEncryptionKey) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let c1s = self.to_rust_type();
        let c2s = other.to_rust_type();
        let ct_strs = c1s
            .into_par_iter()
            .zip(c2s)
            .map(|(c1, c2)| {
                let new_c = Paillier::add(&ek, c1, c2);
                CCiphertext::rust_type2string(&new_c)
            })
            .collect::<Vec<String>>();
        let cts = ct_strs
            .into_iter()
            .map(|ct_str| {
                let ptr = str2ptr(ct_str);
                CCiphertext { ptr }
            })
            .collect();
        Self { cts }
    }

    pub fn sub_ciphertext(&self, other: &Self, cek: &CEncryptionKey) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let n = ek.n;
        let minus_one = n - BigInt::one();
        let minus_c2 = other.scalar_ciphertext_bigint(minus_one, cek);
        let out = self.add_ciphertext(&minus_c2, cek);
        minus_c2.drop();
        out
    }

    pub fn scalar_ciphertext(&self, scalar: u16, cek: &CEncryptionKey) -> Self {
        let scalar = BigInt::from(scalar as u64);
        self.scalar_ciphertext_bigint(scalar, cek)
    }

    pub fn scalar_ciphertext_percent(&self, scalar: u16, cek: &CEncryptionKey) -> Self {
        let inv100 = get_inv_for_ek(cek, 100);
        let scalar_int = BigInt::from(scalar as u64);
        let ek: EncryptionKey = cek.to_rust_type();
        let n = ek.n.clone();
        let new_scalar_int = (scalar_int * inv100) % n;
        self.scalar_ciphertext_bigint(new_scalar_int, cek)
    }

    fn scalar_ciphertext_bigint(&self, scalar: BigInt, cek: &CEncryptionKey) -> Self {
        let ek: EncryptionKey = cek.to_rust_type();
        let cs = self.to_rust_type();
        let ct_strs = cs
            .into_par_iter()
            .map(|c| {
                let new_c = Paillier::mul(&ek, c, RawPlaintext::from(&scalar));
                CCiphertext::rust_type2string(&new_c)
            })
            .collect::<Vec<String>>();
        let cts = ct_strs
            .into_iter()
            .map(|ct_str| {
                let ptr = str2ptr(ct_str);
                CCiphertext { ptr }
            })
            .collect();
        Self { cts }
    }

    pub fn drop(&self) {
        for ct in self.cts.iter() {
            ct.drop();
        }
    }
}
