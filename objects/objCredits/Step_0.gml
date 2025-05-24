
if(keycheck(vk_space))
    rollSpeedT = oRollSpeed * rollSpeedUpFactor;
else
    rollSpeedT = oRollSpeed;

rollSpeed = lerp_a(rollSpeed, rollSpeedT, 0.3);
nowY -= rollSpeed / room_speed;
