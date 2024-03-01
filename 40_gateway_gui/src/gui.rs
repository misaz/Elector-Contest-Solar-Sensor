use std::collections::HashMap;
use std::rc::Rc;
use std::time::Duration;

use chrono::Local;
use gtk::gdk::Screen;
use gtk::glib::{timeout_add, timeout_add_local, ControlFlow};
use gtk::{
    ffi, prelude::*, Application, ApplicationWindow, CssProvider, Grid, Label, StyleContext,
};

use crate::sensor::{Sensor, SensorId};

#[derive(Clone)]
pub struct SensorGui {
	pub id: SensorId,
    pub wrapper: Grid,
    pub lbl_temperature: Label,
    pub lbl_name: Label,
    pub lbl_id: Label,
    pub lbl_battery: Label,
}

impl SensorGui {
    fn _create_lbl_and_add_to_grid(
        wrapper: &Grid,
        text: &str,
        class: &str,
        align: f32,
        x: i32,
        y: i32,
        col_span: i32,
    ) -> Label {
        let lbl = Label::new(Some(text));
        lbl.set_xalign(align);
        lbl.style_context().add_class(class);
        wrapper.attach(&lbl, x, y, col_span, 1);

        lbl
    }

    pub fn new(id: SensorId, name: &str, device_id: &str) -> SensorGui {
        let wrapper = Grid::new();
        wrapper.insert_column(0);
        wrapper.insert_column(1);
        wrapper.insert_row(0);
        wrapper.insert_row(1);
        wrapper.insert_row(2);
        wrapper.set_column_homogeneous(true);
        wrapper.set_expand(true);
        wrapper.style_context().add_class("sensor");

        let lbl_temperature = SensorGui::_create_lbl_and_add_to_grid(
            &wrapper,
            "--.- °C",
            "temperature",
            1.0,
            1,
            0,
            1,
        );
        let lbl_name = SensorGui::_create_lbl_and_add_to_grid(&wrapper, name, "name", 0.0, 0, 0, 1);
        let lbl_id = SensorGui::_create_lbl_and_add_to_grid(
            &wrapper,
            format!("Device ID: {}", device_id).as_str(),
            "sensor-id",
            0.0,
            0,
            1,
            2,
        );
        let lbl_battery = SensorGui::_create_lbl_and_add_to_grid(
            &wrapper,
            "Battery state: --%",
            "battery",
            0.0,
            0,
            2,
            2,
        );

        SensorGui {
			id,
            wrapper,
            lbl_temperature,
            lbl_name,
            lbl_id,
            lbl_battery,
        }
    }

    pub fn set_new_data(&self, temperature: f64, battery: i32) {
		let battery_val = match battery {
			_num if battery < 2100 => 0.0,
			_num if battery > 5000 => 100.0,
			num => 100.0 * (num as f64 - 2100.0) / (5000.0 - 2100.0)
		};

        self.lbl_battery
            .set_text(format!("Battery state: {:.0}%", battery_val).as_str());
        self.lbl_temperature
            .set_text(format!("{:.1}°C", temperature).as_str());
    }
}

#[derive(Clone)]
pub struct Gui {
	pub main: Option<gtk::Box>,
	pub lbl_status: Option<Label>,
	pub grd_sensors: Option<Grid>,
	pub sensor_guis: Vec<SensorGui>,
}

impl Gui {

	pub fn new() -> Gui {
		Gui { main: None, lbl_status: None, grd_sensors: None, sensor_guis: Vec::new() }
	}

    pub fn create<'b>(&mut self, sensors: &'b Vec<Sensor> , app: &Application) {
        let window = ApplicationWindow::builder()
            .application(app)
            // .default_width(800)
            // .default_height(480)
            .title("Solar Temperature Sensor Gateway")
            .build();
        window.fullscreen();

        let css_provider = CssProvider::new();
        let load_result = css_provider.load_from_path("gui-style-internal.css");
        if let Ok(_) = load_result {
            StyleContext::add_provider_for_screen(
                &Screen::default().unwrap(),
                &css_provider,
                ffi::GTK_STYLE_PROVIDER_PRIORITY_USER as u32,
            );
        	// window.style_context().add_provider(&css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER as u32);
        } else {
            println!("Warning: error while loading style file.");
        }

        let main = gtk::Box::new(gtk::Orientation::Vertical, 0);
        window.add(&main);

        let lbl_status = Label::new(Some("Loading"));
        lbl_status.style_context().add_class("status");
        main.pack_start(&lbl_status, false, false, 0);

        let grd_sensors = Grid::new();
        let columns = 2;
        let rows = 3;

        for i in 0..columns {
            grd_sensors.insert_column(i);
        }
        for i in 0..rows {
            grd_sensors.insert_row(i);
        }
        grd_sensors.set_column_homogeneous(true);
        main.pack_start(&grd_sensors, false, false, 0);

        let max = columns * rows;
        let mut index = 0;

        for s in sensors {
            if index >= max {
                break;
            }

            let gui = SensorGui::new(s.id.clone(), s.name.as_str(), format!("{}", s.id).as_str());
            grd_sensors.attach(&gui.wrapper, index % columns, index / columns, 1, 1);
			self.sensor_guis.push(gui);

            index += 1;
        }

        window.show_all();

		self.main = Some(main);
		self.lbl_status = Some(lbl_status);
		self.grd_sensors = Some(grd_sensors);
    }

	pub fn set_sensor_data(&mut self, sensor: &Sensor, temperature: f64, battery: i32) {
		let query = self.sensor_guis.iter().find(|x| x.id == sensor.id);
		let sensor_gui = query.expect("Specified sensor do not corespond to target gui.");
		sensor_gui.set_new_data(temperature, battery);
	}

}
