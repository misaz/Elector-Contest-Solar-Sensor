#!/bin/bash
su -l weston -c "cd /home/$USER && TZ='Europe/Prague'; export TZ; /home/$USER/solar_sensor_gateway_rust"
