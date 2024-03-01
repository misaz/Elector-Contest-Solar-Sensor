use std::io::Cursor;

use byteorder::{LittleEndian, ReadBytesExt};
use chrono::{DateTime, Utc};
use crc::{Crc, CRC_8_SMBUS};
use crate::sensor::SensorId;

#[derive(Debug)]
pub struct Packet {
    pub timestamp: DateTime<Utc>,
    pub payload: [u8; 20],
    pub status: i32,
    pub rssi_sync: i32,
    pub rssi_avg: i32,
    pub freq_err: i32,
}

impl Packet {
    pub fn get_device_id(&self) -> SensorId {
        SensorId {
            id: self.payload[0..4].try_into().unwrap(),
        }
    }
}

#[derive(Debug)]
pub struct Measurement {
	pub has_error: bool,
	pub retransmission_num: u8,
	pub boot_id: u16,
	pub packet_counter: u16,
	pub voltage10: u16,
	pub voltage20: u16,
	pub voltage30: u16,
	pub temp_min_raw: u16,
	pub temp_avg_raw: u16,
	pub temp_max_raw: u16,
}

impl Measurement {

	pub fn from_bytes(device_id: &SensorId, payload: [u8; 16]) ->Result<Measurement, ()> {
		let mut c = Cursor::new(payload);

		let header = c.read_u8().unwrap();
		let crc = c.read_u8().unwrap();
		let boot_id = c.read_u16::<LittleEndian>().unwrap();
		let packet_counter = c.read_u16::<LittleEndian>().unwrap();
		let temp_avg_raw = c.read_u16::<LittleEndian>().unwrap();
		let temp_min_raw = c.read_u16::<LittleEndian>().unwrap();
		let temp_max_raw = c.read_u16::<LittleEndian>().unwrap();
		let voltage_data_raw = c.read_u32::<LittleEndian>().unwrap();

		let has_error = (header & (1 << 6)) != 0;
		let retransmission_num: u8 = (header & (3 << 4)) >> 4;

		let mut crc_test: Vec<u8> = device_id.id.iter().chain(payload.iter()).map(|x| *x).collect();
		crc_test[5] = 0;

		let checksum = Crc::<u8>::new(&CRC_8_SMBUS).checksum(crc_test.as_ref());
		if checksum != crc {
			eprintln!("CRC error: {} != {}", checksum, crc);
			// return Err(());
		}

		let voltage30: u16 = (voltage_data_raw & 0xFFF) as u16;
		let voltage20: u16 = ((voltage_data_raw & 0xFFF000) >> 12) as u16;
		let voltage10: u16 = (((voltage_data_raw & 0xFF000000) >> 24) | (((header as u32) & 0xF) << 8)) as u16;

		Ok(Measurement { 
			has_error, 
			retransmission_num, 
			boot_id,
			packet_counter, 
			voltage10, 
			voltage20, 
			voltage30, 
			temp_min_raw, 
			temp_avg_raw, 
			temp_max_raw }
		)
	}

}
