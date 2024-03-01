#![allow(unused_imports)]

use aes::cipher::KeyInit;
use aes::{
    cipher::{generic_array::GenericArray, BlockDecrypt},
    Aes128,
};
use gtk::glib::{self, clone};
use std::{
    cell::{Cell, RefCell},
    os::unix::process::CommandExt,
    process::Command,
    rc::Rc,
    sync::{
        mpsc::{self, Receiver, TryRecvError},
        Arc,
    },
    time::Duration,
};

use chrono::Local;
use com::com_port_worker;
use gtk::{
    glib::{ffi::g_timer_start, timeout_add, timeout_add_local, ControlFlow},
    prelude::*,
    Application, Label,
};
use gui::Gui;
use packet::Packet;
use sensor::Sensor;

use crate::packet::Measurement;

mod com;
mod gui;
mod packet;
mod sensor;

#[macro_use]
extern crate lazy_static;

fn process_packet(gui: &mut Gui, packet: Packet, sensors: &Arc<Vec<Sensor>>) {
    let payload_arg = packet.payload.map(|x| format!("{:02x}", x)).join("");
    let cmd = Command::new("process_packet.sh").arg(payload_arg).spawn();
    if let Err(err) = cmd {
        eprintln!(
            "Executing packet processing script failed with error: {}",
            err
        )
    }

    let packet_devid = packet.get_device_id();

    if let Some(s) = sensors.iter().find(|x| x.id == packet_devid) {
        let key = GenericArray::from(s.aes_key);
        let engine = Aes128::new(&key);
        let data_copy: [u8; 16] = packet
            .payload
            .iter()
            .map(|x| *x)
            .skip(4)
            .collect::<Vec<u8>>()
            .try_into()
            .unwrap();
        let mut data = GenericArray::from(data_copy);

        engine.decrypt_block(&mut data);

        let mut x: [u8; 16] = data.into_iter().collect::<Vec<u8>>().try_into().unwrap();

        for i in 0..16 {
            x[i] ^= s.aes_iv[i];
        }

        let measurement = Measurement::from_bytes(&s.id, x);

        if measurement.is_err() {
            println!("Received packet is invalid.");
            return;
        }

        let measurement = measurement.unwrap();

        println!("Packet: {:?}", measurement);

		gui.set_sensor_data(s, (measurement.temp_avg_raw as f64) * 0.005, measurement.voltage30 as i32);
    } else {
        println!("Skipping processing packet becase enryption key for device is unavalaible.");
    }
}

fn process_pending_packets(gui: &mut Gui, pipe: &Receiver<Packet>, sensors: &Arc<Vec<Sensor>>) {
    loop {
        match pipe.try_recv() {
            Ok(p) => process_packet(gui, p, sensors),
            Err(TryRecvError::Empty) => break,
            Err(e) => eprintln!("Packet reception is broken: {}", e),
        }
    }
}

fn timer_worker(gui: &mut Gui, pipe: &Receiver<Packet>, sensors: &Arc<Vec<Sensor>>) {
    let dt = format!("{}", Local::now().format("%Y-%m-%d %H:%M:%S"));
    gui.lbl_status.as_ref().unwrap().set_text(dt.as_str());
    process_pending_packets(gui, pipe, sensors);
}

fn main() {
    let app = Application::builder().build();

    app.connect_activate(move |app| {
        let (packets_producer, packets_consumer) = mpsc::channel();
        std::thread::spawn(move || com_port_worker("/dev/ttyACM0", packets_producer));

        let sensors = Arc::new(Sensor::from_file("sensors.txt"));

        for i in &(*sensors) {
            println!(
                "Loaded encryption key for sensor: {} (ID: {})",
                i.name, i.id
            );
        }

        let mut gui = Gui::new();
        gui.create(&sensors, app);

        timeout_add_local(Duration::from_secs(1), move || {
            timer_worker(&mut gui, &packets_consumer, &sensors);
            ControlFlow::Continue
        });
    });

    app.run();
}
