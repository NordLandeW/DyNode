
var dT = global.timeManager.get_delta() / 1000000;

// Update acceleration.
currentAcceleration = lerp_timedep(currentAcceleration, 0, accelerationDamping);

if(mouse_wheel_up()) {
    currentAcceleration = -2 * scrollAcceleration;
} else if(mouse_wheel_down()) {
    currentAcceleration = scrollAcceleration;
}

var targetSpeed = baseSpeed;
if(keyboard_check(vk_space)) {
    targetSpeed = baseSpeed * 3;
}


// Update speed.

currentSpeed = lerp_timedep(currentSpeed, targetSpeed, speedDamping);
currentSpeed += currentAcceleration * dT;

nowY -= currentSpeed * dT;
