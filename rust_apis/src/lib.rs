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

mod batched_ct;
mod ciphertext;
mod keys;
mod utils;
pub use batched_ct::*;
pub use ciphertext::*;
pub use keys::*;
pub use utils::*;


#[no_mangle]
pub extern "C" fn key_gen() -> CKeys {
    CKeys::new()
}

#[no_mangle]
pub extern "C" fn encrypt(cek: &CEncryptionKey, plaintext_ptr: *mut u16) -> CCiphertext {
    let slice = unsafe { slice::from_raw_parts(plaintext_ptr, MSG_SHORT_SIZE) };
    let mut plaintext = [0; MSG_SHORT_SIZE];
    for i in 0..MSG_SHORT_SIZE {
        plaintext[i] = slice[i];
    }
    CCiphertext::encrypt(cek, plaintext)
}

#[no_mangle]
pub extern "C" fn decrypt(cdk: &CDecryptionKey, c: &CCiphertext) -> *mut u16 {
    let plaintext = c.decrypt(cdk);
    //mem::forget(plaintext);
    let mut plaintext = plaintext.to_vec();
    plaintext.shrink_to_fit();
    let ptr = plaintext.as_mut_ptr();
    mem::forget(plaintext);
    /*let text_box = Box::new(plaintext);
    let ptr = Box::into_raw(text_box) as *mut u16;*/
    ptr
}

#[no_mangle]
pub extern "C" fn add_ciphertext(
    cek: &CEncryptionKey,
    c1: &CCiphertext,
    c2: &CCiphertext,
) -> CCiphertext {
    c1.add_ciphertext(c2, cek)
}

#[no_mangle]
pub extern "C" fn sub_ciphertext(
    cek: &CEncryptionKey,
    c1: &CCiphertext,
    c2: &CCiphertext,
) -> CCiphertext {
    c1.sub_ciphertext(c2, cek)
}

#[no_mangle]
pub extern "C" fn scalar_ciphertext(
    cek: &CEncryptionKey,
    c: &CCiphertext,
    scalar: u16,
) -> CCiphertext {
    c.scalar_ciphertext(scalar, cek)
}

#[no_mangle]
pub extern "C" fn scalar_ciphertext_percent(
    cek: &CEncryptionKey,
    c: &CCiphertext,
    scalar: u16,
) -> CCiphertext {
    c.scalar_ciphertext_percent(scalar, cek)
}

#[no_mangle]
pub extern "C" fn batch_encrypt(cek: &CEncryptionKey, plaintext_ptr: *mut u16) -> *const CCiphertext {
    let slice = unsafe { slice::from_raw_parts(plaintext_ptr, MSG_SHORT_SIZE*NUM_CHUNK) };
    let plaintexts = slice.chunks(MSG_SHORT_SIZE).map(|s| {
        let mut plaintext = [0; MSG_SHORT_SIZE];
        for i in 0..MSG_SHORT_SIZE {
            plaintext[i] = s[i];
        }
        plaintext
    }).collect::<Vec<PlainText>>();
    let cts = BatchedCT::encrypt(cek, &plaintexts);
    cts.as_ptr()
}

#[no_mangle]
pub extern "C" fn batch_decrypt(cdk: &CDecryptionKey, cts: *const CCiphertext) -> *mut u16 {
    let batched_ct = BatchedCT::from_ptr(cts);
    let decrypted = batched_ct.decrypt(cdk);
    let mut plaintext = Vec::new();
    for i in 0..NUM_CHUNK {
        plaintext.append(&mut decrypted[i].to_vec());
    }
    let ptr = plaintext.as_mut_ptr();
    mem::forget(plaintext);
    ptr
}

#[no_mangle]
pub extern "C" fn batch_add_ciphertext(
    cek: &CEncryptionKey,
    c1s: *const CCiphertext,
    c2s: *const CCiphertext,
) -> *const CCiphertext {
    let c1s = BatchedCT::from_ptr(c1s);
    let c2s = BatchedCT::from_ptr(c2s);
    let new_ct = c1s.add_ciphertext(&c2s, cek);
    new_ct.as_ptr()
}

#[no_mangle]
pub extern "C" fn batch_sub_ciphertext(
    cek: &CEncryptionKey,
    c1s: *const CCiphertext,
    c2s: *const CCiphertext,
) -> *const CCiphertext {
    let c1s = BatchedCT::from_ptr(c1s);
    let c2s = BatchedCT::from_ptr(c2s);
    let new_ct = c1s.sub_ciphertext(&c2s, cek);
    new_ct.as_ptr()
}

#[no_mangle]
pub extern "C" fn batch_sub_plaintexts(
    plaintext1_ptr: *mut u16,
    plaintext2_ptr: *mut u16,
) -> *mut u16 {
    let slice1:&[u16] = unsafe { slice::from_raw_parts(plaintext1_ptr, MSG_SHORT_SIZE*NUM_CHUNK) };
    let slice2:&[u16] = unsafe { slice::from_raw_parts(plaintext2_ptr, MSG_SHORT_SIZE*NUM_CHUNK) };
    let mut subs = slice1.into_par_iter().zip(slice2).map(|(v1,v2)| v1 - v2).collect::<Vec<u16>>();
    let ptr = subs.as_mut_ptr();
    mem::forget(subs);
    ptr
}

#[no_mangle]
pub extern "C" fn batch_scalar_ciphertext(
    cek: &CEncryptionKey,
    cs: *const CCiphertext,
    scalar: u16,
) -> *const CCiphertext {
    let cs = BatchedCT::from_ptr(cs);
    let new_ct = cs.scalar_ciphertext(scalar, cek);
    new_ct.as_ptr()
}

#[no_mangle]
pub extern "C" fn batch_scalar_ciphertext_percent(
    cek: &CEncryptionKey,
    cs: *const CCiphertext,
    scalar: u16,
) -> *const CCiphertext {
    let cs = BatchedCT::from_ptr(cs);
    let new_ct = cs.scalar_ciphertext_percent(scalar, cek);
    new_ct.as_ptr()
}

#[no_mangle]
pub extern "C" fn batch_free_ciphertext(
    cs: *const CCiphertext,
) {
    let cs = BatchedCT::from_ptr(cs);
    cs.drop();
}


#[cfg(test)]
mod test {
    use super::*;
    use rand::*;

    #[test]
    fn enc_dec_rust_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msg: PlainText = [0; MSG_SHORT_SIZE];
        for i in 0..MSG_SHORT_SIZE {
            msg[i] = rng.gen();
        }
        let c = CCiphertext::encrypt(&keys.ek, msg.clone());
        let decrypted = c.decrypt(&keys.dk);
        assert_eq!(msg, decrypted);
    }

    #[test]
    fn enc_dec_ptr_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msg: PlainText = [0; MSG_SHORT_SIZE];
        for i in 0..MSG_SHORT_SIZE {
            msg[i] = rng.gen();
        }
        let c = CCiphertext::encrypt(&keys.ek, msg.clone());
        let dec_ptr = decrypt(&keys.dk, &c);
        let slice = unsafe { slice::from_raw_parts_mut(dec_ptr, MSG_SHORT_SIZE) };
        assert_eq!(msg, slice);
    }

    #[test]
    fn get_inv_for_ek_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let scalar = rng.gen();
        get_inv_for_ek(&keys.ek, scalar);
    }

    #[test]
    fn add_test() {
        let keys = key_gen();
        let mut msg1: PlainText = [0; MSG_SHORT_SIZE];
        let mut msg2: PlainText = [0; MSG_SHORT_SIZE];
        let mut rng = rand::thread_rng();
        for i in 0..MSG_SHORT_SIZE {
            msg1[i] = rng.gen::<u16>();
            msg2[i] = rng.gen::<u16>();
        }
        let c1 = CCiphertext::encrypt(&keys.ek, msg1.clone());
        let c2 = CCiphertext::encrypt(&keys.ek, msg2.clone());
        let c_add = c1.add_ciphertext(&c2, &keys.ek);
        let decrypted = c_add.decrypt(&keys.dk);
        let u16_max = BigInt::from((u16::MAX as u64) + 1);
        for i in 0..MSG_SHORT_SIZE {
            let correct = (BigInt::from(msg1[i] as u64) + BigInt::from(msg2[i] as u64)) % &u16_max;
            assert_eq!(correct, BigInt::from(decrypted[i] as u64));
        }
    }

    #[test]
    fn sub_test() {
        let keys = key_gen();
        let mut msg1: PlainText = [0; MSG_SHORT_SIZE];
        let mut msg2: PlainText = [0; MSG_SHORT_SIZE];
        let mut rng = rand::thread_rng();
        for i in 0..MSG_SHORT_SIZE {
            msg1[i] = rng.gen::<u16>();
            msg2[i] = msg1[i];
        }
        let c1 = CCiphertext::encrypt(&keys.ek, msg1.clone());
        let c2 = CCiphertext::encrypt(&keys.ek, msg2.clone());
        let c_sub = c1.sub_ciphertext(&c2, &keys.ek);
        let decrypted = c_sub.decrypt(&keys.dk);
        for i in 0..MSG_SHORT_SIZE {
            assert_eq!(BigInt::zero(), BigInt::from(decrypted[i] as u64));
        }
    }

    #[test]
    fn sub_100_test() {
        let keys = key_gen();
        let mut msg1: PlainText = [0; MSG_SHORT_SIZE];
        let mut msg2: PlainText = [0; MSG_SHORT_SIZE];
        let mut rng = rand::thread_rng();
        for i in 0..MSG_SHORT_SIZE {
            msg1[i] = rng.gen::<u16>();
            msg2[i] = msg1[i];
        }
        let c1 = CCiphertext::encrypt(&keys.ek, msg1.clone());
        let c1 = c1.scalar_ciphertext_percent(100, &keys.ek);
        let c2 = CCiphertext::encrypt(&keys.ek, msg2.clone());
        let c2 = c2.scalar_ciphertext_percent(100, &keys.ek);
        let c_sub = c1.sub_ciphertext(&c2, &keys.ek);
        let decrypted = c_sub.decrypt(&keys.dk);
        for i in 0..MSG_SHORT_SIZE {
            assert_eq!(BigInt::zero(), BigInt::from(decrypted[i] as u64));
        }
    }

    #[test]
    fn scalar_test() {
        let keys = key_gen();
        let mut msg1: PlainText = [0; MSG_SHORT_SIZE];
        let mut rng = rand::thread_rng();
        for i in 0..MSG_SHORT_SIZE {
            msg1[i] = rng.gen::<u16>();
        }
        let scalar: u16 = rng.gen::<u16>();
        let c1 = CCiphertext::encrypt(&keys.ek, msg1.clone());
        let c_scalar = c1.scalar_ciphertext(scalar, &keys.ek);
        let decrypted = c_scalar.decrypt(&keys.dk);
        let u16_max = BigInt::from((u16::MAX as u64) + 1);
        for i in 0..MSG_SHORT_SIZE {
            let correct = (BigInt::from(msg1[i] as u64) * BigInt::from(scalar as u64)) % &u16_max;
            assert_eq!(correct, BigInt::from(decrypted[i] as u64));
        }
    }

    #[test]
    fn scalar_100_test() {
        let keys = key_gen();
        let mut msg1: [u16; MSG_SHORT_SIZE] = [0; MSG_SHORT_SIZE];
        let mut rng = rand::thread_rng();
        for i in 0..MSG_SHORT_SIZE {
            msg1[i] = rng.gen();
        }
        let scalar1: u16 = rng.gen::<u16>() % 100;
        let scalar2: u16 = 100 - scalar1;
        let c1 = CCiphertext::encrypt(&keys.ek, msg1.clone());
        let c_scalar1 = c1.scalar_ciphertext_percent(scalar1, &keys.ek);
        let c_scalar2 = c1.scalar_ciphertext_percent(scalar2, &keys.ek);
        let c_add = c_scalar1.add_ciphertext(&c_scalar2, &keys.ek);
        let decrypted = c_add.decrypt(&keys.dk);
        assert_eq!(msg1, decrypted);
    }

    #[test]
    fn batch_encrypt_decrypt_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msgs = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs.push(plaintext);
        }
        let c = BatchedCT::encrypt(&keys.ek, &msgs[..]);
        let decrypted = c.decrypt(&keys.dk);
        assert_eq!(msgs, decrypted);
    }

    #[test]
    fn batch_add_ciphertext_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msgs1 = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs1.push(plaintext);
        }
        let mut msgs2 = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs2.push(plaintext);
        }
        let c1 = BatchedCT::encrypt(&keys.ek, &msgs1[..]);
        let c2 = BatchedCT::encrypt(&keys.ek, &msgs2[..]);
        let add_c = c1.add_ciphertext(&c2, &keys.ek);
        let decrypted = add_c.decrypt(&keys.dk);
        let u16_max = BigInt::from((u16::MAX as u64) + 1);
        for i in 0..NUM_CHUNK {
            let msg1 = msgs1[i];
            let msg2 = msgs2[i];
            let dec = decrypted[i];
            for j in 0..MSG_SHORT_SIZE {
                let correct = (BigInt::from(msg1[j] as u64) + BigInt::from(msg2[j] as u64)) % &u16_max;
                assert_eq!(correct, BigInt::from(dec[j] as u64));
            }
        }
    }

    #[test]
    fn batch_sub_ciphertext_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msgs1 = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs1.push(plaintext);
        }
        let mut msgs2 = msgs1.clone();
        let c1 = BatchedCT::encrypt(&keys.ek, &msgs1[..]);
        let c2 = BatchedCT::encrypt(&keys.ek, &msgs2[..]);
        let sub_c = c1.sub_ciphertext(&c2, &keys.ek);
        let decrypted = sub_c.decrypt(&keys.dk);
        let u16_max = BigInt::from((u16::MAX as u64) + 1);
        for i in 0..NUM_CHUNK {
            let dec = decrypted[i];
            for j in 0..MSG_SHORT_SIZE {
                assert_eq!(BigInt::zero(), BigInt::from(dec[j] as u64));
            }
        }
    }

    #[test]
    fn batch_sub_100_ciphertext_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msgs1 = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs1.push(plaintext);
        }
        let mut msgs2 = msgs1.clone();
        let c1 = BatchedCT::encrypt(&keys.ek, &msgs1[..]);
        let c1 = c1.scalar_ciphertext_percent(100, &keys.ek);
        let c2 = BatchedCT::encrypt(&keys.ek, &msgs2[..]);
        let c2 = c2.scalar_ciphertext_percent(100, &keys.ek);
        let sub_c = c1.sub_ciphertext(&c2, &keys.ek);
        let decrypted = sub_c.decrypt(&keys.dk);
        let u16_max = BigInt::from((u16::MAX as u64) + 1);
        for i in 0..NUM_CHUNK {
            let dec = decrypted[i];
            for j in 0..MSG_SHORT_SIZE {
                assert_eq!(BigInt::zero(), BigInt::from(dec[j] as u64));
            }
        }
    }


    #[test]
    fn batch_scalar_100_test() {
        let keys = key_gen();
        let mut rng = thread_rng();
        let mut msgs1 = Vec::new();
        for i in 0..NUM_CHUNK {
            let mut plaintext:PlainText = [0; MSG_SHORT_SIZE];
            for j in 0..MSG_SHORT_SIZE {
                plaintext[j] = rng.gen();
            }
            msgs1.push(plaintext);
        }
        let scalar1: u16 = rng.gen::<u16>() % 100;
        let scalar2: u16 = 100 - scalar1;
        let c = BatchedCT::encrypt(&keys.ek, &msgs1[..]);
        let scalar_c1 = c.scalar_ciphertext_percent(scalar1, &keys.ek);
        let scalar_c2 = c.scalar_ciphertext_percent(scalar2, &keys.ek);
        let add_c = scalar_c1.add_ciphertext(&scalar_c2, &keys.ek);
        let decrypted = add_c.decrypt(&keys.dk);
        assert_eq!(msgs1, decrypted);
    }
}
