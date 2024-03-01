use std::{io::{BufRead, BufReader}, sync::mpsc::Sender, time::Duration};
use chrono::Utc;
use regex::Regex;

use crate::packet::Packet;

fn parse_com_line(line: &str) -> Result<Packet, &'static str> {
    lazy_static! {
        static ref PARSE_REGEX: Regex = Regex::new(r"(?m)([0-9a-fA-F]{40}) STATUS=([\d-]+) RSSI_SYNC=([\d-]+) RSSI_AVG=([\d-]+) FREQ_ERR=([\d-]+)").unwrap();
    }

    let now = Utc::now();

    if let Some(m) = PARSE_REGEX.captures(line) {
        let payload_raw = m.get(1).unwrap().as_str();
        let payload: [u8; 20] = (0..40)
            .step_by(2)
            .map(|i| &payload_raw[i..i + 2])
            .map(|x| u8::from_str_radix(x, 16).unwrap())
            .collect::<Vec<u8>>()
            .try_into()
            .unwrap();

        let status: i32 = i32::from_str_radix(m.get(2).unwrap().as_str(), 10).unwrap();
        let rssi_sync: i32 = i32::from_str_radix(m.get(3).unwrap().as_str(), 10).unwrap();
        let rssi_avg: i32 = i32::from_str_radix(m.get(4).unwrap().as_str(), 10).unwrap();
        let freq_err: i32 = i32::from_str_radix(m.get(5).unwrap().as_str(), 10).unwrap();

        Ok(Packet {
            timestamp: now,
            payload,
            status,
            rssi_sync,
            rssi_avg,
            freq_err,
        })
    } else {
        Err("Unable to parse received line.")
    }
}

pub fn com_port_worker(port_name: &str, output_pipe: Sender<Packet>) {
    'infloop: loop {
        let open_result = serialport::new(port_name, 115200)
            .timeout(Duration::from_secs(3))
            .open();
        if let Err(err) = open_result {
            eprintln!("Failed to open COM port. Details: {}", err);
            std::thread::sleep(Duration::from_secs(10));
            continue;
        }

        let port = open_result.unwrap();
        let buffered_port = BufReader::new(port);
        for line in buffered_port.lines() {
            match line {
                Ok(line) => {
                    println!("Received line: {}", line);
                    let parsed = parse_com_line(&line);
                    match parsed {
                        Ok(packet) => {
                            println!("parsed as {:?}", packet);
                            output_pipe.send(packet).unwrap();
                        }
                        Err(err) => {
                            eprintln!(
                                "Failed to parse received line '{}''. Details: {}",
                                line, err
                            );
                            continue 'infloop;
                        }
                    }
                }
                Err(err) if err.kind() == std::io::ErrorKind::TimedOut => {
                    continue;
                }
                Err(err) => {
                    eprintln!(
                        "Failed to read from COM port. Kind: {:?} Details: {:?}",
                        err.kind(),
                        err
                    );
                    continue 'infloop;
                }
            }
        }
    }
}