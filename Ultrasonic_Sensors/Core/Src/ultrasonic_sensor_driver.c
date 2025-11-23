#include "ultrasonic_sensor_driver.h"

static ultrasonic_Handle_t* sensors[ULTRASONIC_MAX_SENSORS];
static uint8_t sensor_count = 0;

void Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor)
{
	if(sensor_count < ULTRASONIC_MAX_SENSORS){
		sensors[sensor_count++] = ultrasonic_sensor;
	}
	ultrasonic_sensor->echo_state = WAITING_FOR_RISING;
	ultrasonic_sensor->ultrasonic_ready = 0;
    HAL_TIM_IC_Start_IT(ultrasonic_sensor->htim_echo, ultrasonic_sensor->echo_channel);
}
ultrasonic_status_t Ultrasonic_Trigger(ultrasonic_Handle_t* ultrasonic_sensor){
	if(ultrasonic_sensor->ultrasonic_ready ||
			ultrasonic_sensor->echo_state == WAITING_FOR_FALLING){
		return ULTRASONIC_BUSY;
	}
	else{
		ultrasonic_sensor->ultrasonic_ready = 0;
		ultrasonic_sensor->t_fall = 0;
		ultrasonic_sensor->t_rise = 0;
		__HAL_TIM_DISABLE(ultrasonic_sensor->htim_trig);  // stop safe
		__HAL_TIM_SET_COUNTER(ultrasonic_sensor->htim_trig, 0);  // reset CNT
		HAL_TIM_PWM_Start(ultrasonic_sensor->htim_trig, ultrasonic_sensor->trig_channel);
		__HAL_TIM_ENABLE(ultrasonic_sensor->htim_trig);

	}

	return ULTRASONIC_OK;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t i = 0; i < sensor_count; i++)
    {
        ultrasonic_Handle_t* s = sensors[i];

        // Check if this interrupt is for this sensor’s echo timer
        if (htim == s->htim_echo)
        {
            // Get which channel triggered
            uint32_t active_channel = htim->Channel;

            if ((active_channel == HAL_TIM_ACTIVE_CHANNEL_1 && s->echo_channel == TIM_CHANNEL_1) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_2 && s->echo_channel == TIM_CHANNEL_2) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_3 && s->echo_channel == TIM_CHANNEL_3) ||
                (active_channel == HAL_TIM_ACTIVE_CHANNEL_4 && s->echo_channel == TIM_CHANNEL_4))
            {
                // RISING edge
                if (s->echo_state == WAITING_FOR_RISING)
                {
                    s->t_rise = __HAL_TIM_GET_COMPARE(s->htim_echo, s->echo_channel);

                    TIM_IC_InitTypeDef cfg = {0};
                    cfg.ICPolarity  = TIM_INPUTCHANNELPOLARITY_FALLING;
                    cfg.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    cfg.ICFilter    = 0;
                    cfg.ICPrescaler = TIM_ICPSC_DIV1;
                    HAL_TIM_IC_ConfigChannel(s->htim_echo, &cfg, s->echo_channel);
                    HAL_TIM_IC_Start_IT(s->htim_echo, s->echo_channel);

                    s->echo_state = WAITING_FOR_FALLING;
                }
                // FALLING edge
                else if (s->echo_state == WAITING_FOR_FALLING)
                {
                    s->t_fall = __HAL_TIM_GET_COMPARE(s->htim_echo, s->echo_channel);

                    TIM_IC_InitTypeDef cfg = {0};
                    cfg.ICPolarity  = TIM_INPUTCHANNELPOLARITY_RISING;
                    cfg.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    cfg.ICFilter    = 0;
                    cfg.ICPrescaler = TIM_ICPSC_DIV1;
                    HAL_TIM_IC_ConfigChannel(s->htim_echo, &cfg, s->echo_channel);
                    HAL_TIM_IC_Start_IT(s->htim_echo, s->echo_channel);

                    s->ultrasonic_ready = 1;
                    s->echo_state = WAITING_FOR_RISING;
                }
            }
        }
    }
}

uint8_t Ultrasonic_IsReady(ultrasonic_Handle_t* ultrasonic_sensor){
	return ultrasonic_sensor->ultrasonic_ready;
}
float Ultrasonic_GetDistance(ultrasonic_Handle_t* ultrasonic_sensor){
	if(Ultrasonic_IsReady(ultrasonic_sensor)==0){
		return NAN;
	}
	else{
		uint16_t time_difference = ultrasonic_sensor->t_fall - ultrasonic_sensor->t_rise ;
		ultrasonic_sensor->ultrasonic_ready = 0;
		float distance = (time_difference / 2.0f) * 0.0343f;
		return distance;
	}
}
