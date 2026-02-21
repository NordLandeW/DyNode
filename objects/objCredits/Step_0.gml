
var dT = global.timeManager.get_delta() / 1000000;

// Update acceleration.
currentAcceleration = lerp_timedep(currentAcceleration, 0, accelerationDamping);
currentRenewAcceleration = lerp_timedep(currentRenewAcceleration, 0, renewAccelerationDamping);

if(mouse_wheel_up()) {
    currentAcceleration += -1.5 * scrollAcceleration;
} else if(mouse_wheel_down()) {
    currentAcceleration += scrollAcceleration;
}

var targetSpeed = baseSpeed;
if(keyboard_check(vk_space)) {
    targetSpeed = baseSpeed * 3;
}


// Update speed.

currentSpeed = lerp_timedep(currentSpeed, targetSpeed, speedDamping);
currentSpeed += (currentAcceleration + currentRenewAcceleration) * dT;

nowY -= currentSpeed * dT;
