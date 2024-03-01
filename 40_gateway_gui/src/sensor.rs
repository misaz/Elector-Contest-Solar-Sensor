use std::{
    fmt::Display,
    fs::File,
    io::{BufRead, BufReader},
    path::Path,
    str,
};

#[derive(PartialEq, Eq, Hash, Clone)]
pub struct SensorId {
    pub id: [u8; 4],
}

impl Display for SensorId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{:02x}{:02x}{:02x}{:02x}",
            self.id[0], self.id[1], self.id[2], self.id[3]
        )
    }
}

#[derive(Hash, Eq, PartialEq)]
pub struct Sensor {
    pub id: SensorId,
    pub aes_key: [u8; 16],
    pub aes_iv: [u8; 16],
    pub name: String,

    pub last_temperature: Option<i32>,
}

fn parse_128bit_str(input: &str) -> Result<[u8; 16], ()> {
    let attemp = (0..16)
        .map(|i| u8::from_str_radix(&input[(i * 2)..(i * 2 + 2)], 16))
        .collect::<Vec<Result<u8, _>>>();

    let first_error = attemp.iter().find(|x| x.is_err());
    if let Some(_) = first_error {
        return Err(());
    }

	let output: [u8; 16] = attemp.into_iter().map(|x| x.unwrap()).collect::<Vec<u8>>().try_into().unwrap();

    Ok(output)
}

impl Sensor {
    pub fn from_file<P: AsRef<Path>>(filename: P) -> Vec<Sensor> {
        let file = BufReader::new(File::open(filename).expect("Error reading sensors file."));

        let mut vec: Vec<Sensor> = Vec::new();

        for l in file.lines() {
            let l = l.expect("Error reading sensors file.");

            if l.trim().is_empty() {
                continue;
            }

            let parts: Vec<&str> = l.split(" ").collect();

            if parts.len() < 3 {
                eprintln!("Invalid sensor line '{}'. Format is {{device_id}} {{device_enc_key}} {{friendly name}}", l);
                continue;
            }

            let id_raw = parts[0];
            if id_raw.len() != 8 {
                eprintln!(
                    "Invalid sensor device id '{}'. Must be 4-byte hexadecimal number.",
                    id_raw
                );
                continue;
            }
            let id: [u8; 4] = (0..4)
                .map(|i| u8::from_str_radix(&id_raw[(i * 2)..(i * 2 + 2)], 16).unwrap())
                .collect::<Vec<u8>>()
                .try_into()
                .unwrap();

            let key_str = parts[1];
            let iv_str = parts[2];
            let name = parts[3..].join(" ");

            let key = parse_128bit_str(key_str);
            let iv = parse_128bit_str(iv_str);

            if let Err(_) = key {
                eprintln!("Invalid encryption key '{}'. Must be 16-byte hexadecimal number.", key_str);
				continue;
            }
			
            if let Err(_) = iv {
                eprintln!("Invalid encryption key '{}'. Must be 16-byte hexadecimal number.", key_str);
				continue;
            }

            vec.push(Sensor {
                id: SensorId { id },
                aes_key: key.unwrap(),
                aes_iv: iv.unwrap(),
                name: name,
                last_temperature: None,
            });
        }

        vec
    }
}
