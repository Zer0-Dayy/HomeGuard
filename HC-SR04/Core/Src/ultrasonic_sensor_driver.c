#include "ultrasonic_sensor_driver.h"

static ultrasonic_Handle_t* sensor = NULL;

void Ultrasonic_Sensor_Init(ultrasonic_Handle_t* ultrasonic_sensor)
{
	sensor = ultrasonic_sensor;
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
	if(sensor == NULL){
		return;
	}
	else if(htim == sensor->htim_echo){
    	if(sensor->echo_state == WAITING_FOR_RISING){
    		sensor->t_rise = __HAL_TIM_GET_COMPARE(sensor->htim_echo,sensor->echo_channel);
    		TIM_IC_InitTypeDef sConfig= {0};
    		sConfig.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    		sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
    		sConfig.ICFilter = 0;
			sConfig.ICPrescaler = TIM_ICPSC_DIV1;
			HAL_TIM_IC_ConfigChannel(sensor->htim_echo,&sConfig,sensor->echo_channel);
			HAL_TIM_IC_Start_IT(sensor->htim_echo,sensor->echo_channel);
    		sensor->echo_state = WAITING_FOR_FALLING;
    	}
    	else if(sensor->echo_state == WAITING_FOR_FALLING){

    	    		sensor->t_fall = __HAL_TIM_GET_COMPARE(sensor->htim_echo,sensor->echo_channel);
    	    		TIM_IC_InitTypeDef sConfig = {0};
    	    		sConfig.ICFilter = 0;
    	    		sConfig.ICPolarity =  TIM_INPUTCHANNELPOLARITY_RISING;
    	    		sConfig.ICPrescaler = TIM_ICPSC_DIV1;
    	    		sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
    	    		HAL_TIM_IC_ConfigChannel(sensor->htim_echo,&sConfig,sensor->echo_channel);
    	    		HAL_TIM_IC_Start_IT(sensor->htim_echo,sensor->echo_channel);
    	    		sensor->ultrasonic_ready = 1;
    	    		sensor->echo_state = WAITING_FOR_RISING;
    	    	}
    }
	else{
		return;
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
