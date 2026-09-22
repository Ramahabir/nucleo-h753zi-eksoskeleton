#include "main.h"
#include "can.h"
#include "robstride_control.h"
#include <string.h>

motor_feedback_t mf;
/*******************************************************************************
* @����     		: float������תint��
* @����1        : ��Ҫת����ֵ
* @����2        : x����Сֵ
* @����3        : x�����ֵ
* @����4        : ��Ҫת���Ľ�����
* @����ֵ 			: ʮ���Ƶ�int������
* @����  				: None
*******************************************************************************/
static int float_to_uint(float x, float x_min, float x_max, int bits)
{
	float span = x_max - x_min;
	float offset = x_min;
	if(x > x_max) x=x_max;
	else if(x < x_min) x= x_min;
	return (int) ((x-offset)*((float)((1<<bits)-1))/span);
}

/*******************************************************************************
* @����     		: uint16_t��תfloat�͸�����
* @����1        : ��Ҫת����ֵ
* @����2        : x����Сֵ
* @����3        : x�����ֵ
* @����4        : ��Ҫת���Ľ�����
* @����ֵ 			: ʮ���Ƶ�float�͸�����
* @����  				: None
*******************************************************************************/
static float uint16_to_float(uint16_t x, float min, float max)
{
    return ((float)x) / 65535.0f * (max - min) + min;
}

/*******************************************************************************
* @����     		: uint8_t����תfloat������
* @����        	: ��Ҫת��������
* @����ֵ 			: ʮ���Ƶ�float�͸�����
* @����  				: None
*******************************************************************************/
float Byte_to_float(uint8_t* bytedata)  
{  
	uint32_t data = bytedata[7]<<24|bytedata[6]<<16|bytedata[5]<<8|bytedata[4];
	float data_float; memcpy(&data_float, &data, sizeof(float));
  return data_float;  
}  

/*******************************************************************************
* @����     		: RobStride���ʹ�� ��ͨ������3��
* @����         : None
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_motor_enable(void)
{
	txCanIdEx.mode = MOTOR_IN;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	memset(tx_data, 0, 8);
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride���ʧ�� ��ͨ������4��
* @����         : �Ƿ��������λ��0����� 1�����
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_motor_reset(void)
{
	txCanIdEx.mode = MOTOR_RESET;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	for(uint8_t i=0;i<8;i++)
	{
		tx_data[i]=0;
	}
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride���д����� ��ͨ������18��
* @����1        : ������ַ
* @����2        : ������ֵ
* @����3        : ѡ���Ǵ������ģʽ ������������ ��Set_mode���ÿ���ģʽ Set_parameter���ò�����
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void Set_RobStride_Motor_parameter(uint16_t Index, float Value, char Value_mode)
{
	txCanIdEx.mode = MOTOR_PARAWRITE;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	tx_data[0] = Index;
	tx_data[1] = Index>>8;
	tx_data[2] = 0x00;
	tx_data[3] = 0x00;	
	if (Value_mode == 'p')
	{
		memcpy(&tx_data[4],&Value,4);
	}
	else if (Value_mode == 'j')
	{
		mf.motorMode=move_control_mode;
		tx_data[4] = (uint8_t)Value;
		tx_data[5] = 0x00;	
		tx_data[6] = 0x00;	
		tx_data[7] = 0x00;	
	}
  can_txd();
	HAL_Delay(1);
}

/*******************************************************************************
* @����     		: RobStride�������������ȡ ��ͨ������17��
* @����         : ������ַ
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void Get_RobStride_Motor_parameter(uint16_t Index)
{
	txCanIdEx.mode = MOTOR_PARAREAD;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	tx_data[0] = Index;
	tx_data[1] = Index>>8;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride����˿�ģʽ  ��ͨ������1��
* @����1        : ���أ�-4Nm~4Nm��
* @����2        : Ŀ��Ƕ�(-4��~4��)
* @����3        : Ŀ����ٶ�(-30rad/s~30rad/s)
* @����4        : Kp(0.0~500.0)
* @����5        : Kp(0.0~5.0)
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStrideMotor_move_control(float torque, float MechPosition,float speed, float kp, float kd)
{
	if(mf.mms==running)
	{
		RobStride_motor_reset();
	}
	Set_RobStride_Motor_parameter(0x7005, move_control_mode,Set_mode);
	RobStride_motor_enable();
	txCanIdEx.mode = MOTOR_CTRL;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = float_to_uint(torque,T_MIN,T_MAX,16);
	txMsg.DLC = 8;
	tx_data[0]=float_to_uint(MechPosition,P_MIN,P_MAX,16)>>8;
	tx_data[1]=float_to_uint(MechPosition,P_MIN,P_MAX,16);
	tx_data[2]=float_to_uint(speed,V_MIN,V_MAX,16)>>8;
	tx_data[3]=float_to_uint(speed,V_MIN,V_MAX,16);
	tx_data[4]=float_to_uint(kp,KP_MIN,KP_MAX,16)>>8;
	tx_data[5]=float_to_uint(kp,KP_MIN,KP_MAX,16);
	tx_data[6]=float_to_uint(kd,KD_MIN,KD_MAX,16)>>8;
	tx_data[7]=float_to_uint(kd,KD_MIN,KD_MAX,16);
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride����ٶ�ģʽ 
* @����1        : Ŀ����ٶ�(-30rad/s~30rad/s)
* @����2        : Ŀ���������(0~23A)
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_Motor_Speed_control(float limit_cur,float Speed_acc,float Speed)
{
	if(mf.mms==running)
	{
		RobStride_motor_reset();
	}
	Set_RobStride_Motor_parameter(0X7005, Speed_control_mode, Set_mode);		//���õ��ģʽ
	RobStride_motor_enable();
		
	Set_RobStride_Motor_parameter(0X7018, limit_cur, Set_parameter);
	Set_RobStride_Motor_parameter(0X7022, Speed_acc, Set_parameter);	
	Set_RobStride_Motor_parameter(0X700A, Speed, Set_parameter);
}

/*******************************************************************************
* @����     		: RobStride���λ��ģʽ(PP�岹λ��ģʽ����)
* @����1        : Ŀ����ٶ�(-30rad/s~30rad/s)
* @����2        : Ŀ��Ƕ�(-4��~4��)
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_Motor_Pos_PP_control(float Speed, float Speed_acc, float Angle)
{
	if(mf.mms==running)
	{
		RobStride_motor_reset();
	}
	Set_RobStride_Motor_parameter(0X7005, Pos_PP_control_mode, Set_mode);		//���õ��ģʽ
	RobStride_motor_enable();
		
	Set_RobStride_Motor_parameter(0X7024, Speed, Set_parameter);
	Set_RobStride_Motor_parameter(0X7025, Speed_acc, Set_parameter);	
	Set_RobStride_Motor_parameter(0X7016, Angle, Set_parameter);
}

/*******************************************************************************
* @����     		: RobStride���λ��ģʽ(CSPλ��ģʽ����)
* @����1        : Ŀ��Ƕ�(-4��~4��)
* @����2        : Ŀ����ٶ�(0rad/s~44rad/s)
* @����ֵ 				: void
* @����  				: None
*******************************************************************************/
void RobStride_Motor_Pos_CSP_control(float Speed, float Angle)
{
	if(mf.mms==running)
	{
		RobStride_motor_reset();
	}
	Set_RobStride_Motor_parameter(0X7005, Pos_CSP_control_mode, Set_mode);		//���õ��ģʽ
	RobStride_motor_enable();
		
	Set_RobStride_Motor_parameter(0X7017, Speed, Set_parameter);
	Set_RobStride_Motor_parameter(0X7016, Angle, Set_parameter);
}

/*******************************************************************************
* @����     		: RobStride�������ģʽ
* @����         : Ŀ�����(-23~23A)
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_Motor_current_control(float cuttent)
{
	if(mf.mms==running)
	{
		RobStride_motor_reset();
	}
	Set_RobStride_Motor_parameter(0X7005, Elect_control_mode, Set_mode);		//���õ��ģʽ
	RobStride_motor_enable();
		
	Set_RobStride_Motor_parameter(0X7006, cuttent, Set_parameter);
}

/*******************************************************************************
* @����     		: RobStride������û�е��� ��ͨ������6��
* @����         : None
* @����ֵ 			: void
* @����  				: ��ѵ�ǰ���λ����Ϊ��е��λ�� ����ʧ�ܵ��, ��ʹ�ܵ��
*******************************************************************************/
void RobStride_Set_ZreoPos(void)
{
	RobStride_motor_reset();
	txCanIdEx.mode = MOTOR_ZERO;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	tx_data[0] = 1;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride�������CAN_ID ��ͨ������7��
* @����         : �޸ĺ�Ԥ�裩CANID
* @����ֵ 			: void
* @����  				: None
*******************************************************************************/
void RobStride_Set_CAN_ID(uint8_t Set_CAN_ID)
{
	RobStride_motor_reset();
	txCanIdEx.mode = MOTOR_ID;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = Set_CAN_ID<<8|MASTER_ID;
	txMsg.DLC = 8;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride������ݱ��� ��ͨ������22��
* @����      		: None
* @����ֵ 				: void
* @����  				: ��ѵ�ǰ������д����е�����дΪĬ��ֵ�������ϵ���������Ϊ��ָ������ʱ�Ĳ���
*******************************************************************************/
void RobStride_Motor_MotorDataSave(void)
{
	txCanIdEx.mode = MOTOR_DataSave;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	tx_data[0] = 0x01;
	tx_data[1] = 0x02;
	tx_data[2] = 0x03;
	tx_data[3] = 0x04;
	tx_data[4] = 0x05;
	tx_data[5] = 0x06;
	tx_data[6] = 0x07;
	tx_data[7] = 0x08;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride����������޸� ��ͨ������23��
* @����      		: ������ģʽ:	 01��1M��
									02��500K��
									03��250K��
									04��125K��
* @����ֵ 				: void
* @����  				: ������������޸�Ϊ��Ӧ��ֵ���������Ϊ01���������޸�Ϊ1M
*******************************************************************************/
void RobStride_Motor_BaudRateChange(uint8_t F_CMD)
{
	txCanIdEx.mode = MOTOR_BAUD;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	tx_data[0] = 0x01;
	tx_data[1] = 0x02;
	tx_data[2] = 0x03;
	tx_data[3] = 0x04;
	tx_data[4] = 0x05;
	tx_data[5] = 0x06;
	tx_data[6] = F_CMD;
	tx_data[7] = 0x08;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride��������ϱ����� ��ͨ������24��
* @����      		: �ϱ�ģʽ��	00���رգ�
														01��������
* @����ֵ 				: void
* @����  				: ����/�ر� ��������ϱ���Ĭ���ϱ�����Ϊ10ms
*******************************************************************************/
void RobStride_Motor_ProtactiveEscalationSet(uint8_t F_CMD)
{
	txCanIdEx.mode = MOTOR_Proactive;
	txCanIdEx.id = CAN_ID;
	txCanIdEx.res = 0;
	txCanIdEx.data = MASTER_ID;
	txMsg.DLC = 8;
	tx_data[0] = 0x01;
	tx_data[1] = 0x02;
	tx_data[2] = 0x03;
	tx_data[3] = 0x04;
	tx_data[4] = 0x05;
	tx_data[5] = 0x06;
	tx_data[6] = F_CMD;
	tx_data[7] = 0x08;
	can_txd();
}

/*******************************************************************************
* @����     		: RobStride���Э���޸� ��ͨ������25��
* @����      		: Э�����ͣ�		00��˽��Э�飩
										01��Canopen��
										02��MITЭ�飩
* @����ֵ 				: void
* @����  				: None
*******************************************************************************/
void RobStride_Motor_MIT_ModeSet(uint8_t F_CMD)
{
	if (F_CMD == 0)//MITЭ�黻��˽��Э����
	{
		mittxCanIdEx.id = CAN_ID;
		MITtxMsg.DLC = 8;
		tx_data[0] = 0xFF;
		tx_data[1] = 0xFF;
		tx_data[2] = 0xFF;
		tx_data[3] = 0xFF;
		tx_data[4] = 0xFF;
		tx_data[5] = 0xFF;
		tx_data[6] = F_CMD;
		tx_data[7] = 0xFD;
		can_MIT_txd();
	}
	else if(F_CMD == 2)//˽��Э���л���MITЭ��
	{
		txCanIdEx.mode = MOTOR_MODESTE;
		txCanIdEx.id = CAN_ID;
		txCanIdEx.res = 0;
		txCanIdEx.data = MASTER_ID;
		txMsg.DLC = 8;
		tx_data[0] = 0x01;
		tx_data[1] = 0x02;
		tx_data[2] = 0x03;
		tx_data[3] = 0x04;
		tx_data[4] = 0x05;
		tx_data[5] = 0x06;
		tx_data[6] = F_CMD;
		tx_data[7] = 0x08;
		can_txd();
	}
}

//MITģʽʹ��
void RobStride_Motor_MIT_enable(void)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = 0xFF;
	tx_data[7] = 0xFC;
	can_MIT_txd();
}

//MITģʽʧ��
void RobStride_Motor_MIT_reset(void)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = 0xFF;
	tx_data[7] = 0xFD;
	can_MIT_txd();
}

//������󼰶�ȡ�쳣״̬
void RobStride_Motor_MIT_ClearOrCheckError(uint8_t F_CMD)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = F_CMD;//���� F_CMD �ֽ�Ϊ 0xFF ʱ����ʾ������ǰ���쳣��Ϊ�����κ���ֵʱ�����ڻظ��е� BYTE1 �лش�����ֵ
	tx_data[7] = 0xFD;
	can_MIT_txd();
}

//MIT��������ģʽ
void RobStride_Motor_MIT_SetMotorType(uint8_t F_CMD)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = F_CMD;//���� F_CMD �ֽ�Ϊ����ģʽ���� 0 Ϊ MIT ģʽ��Ĭ�ϣ�1 Ϊλ��ģʽ2 Ϊ�ٶ�ģʽ
	tx_data[7] = 0xFC;
	can_MIT_txd();
}

//MIT���õ��ID
void RobStride_Motor_MIT_SetMotorId(uint8_t F_CMD)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = F_CMD;//���� F_CMD �ֽ�ΪĿ���޸ĵĵ�� id
	tx_data[7] = 0xFA;
	can_MIT_txd();
}

//MIT����ģʽ
void RobStride_Motor_MIT_Control(float Angle, float Speed, float Kp, float Kd, float Torque)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = float_to_uint(Angle, P_MIN,P_MAX, 16)>>8;
	tx_data[1] = float_to_uint(Angle, P_MIN,P_MAX, 16);
	tx_data[2] = float_to_uint(Speed, V_MIN,V_MAX, 12)>>4;
	tx_data[3] = float_to_uint(Speed, V_MIN,V_MAX, 12)<<4 | float_to_uint(Kp, KP_MIN, KP_MAX, 12)>>8;
	tx_data[4] = float_to_uint(Kp, KP_MIN, KP_MAX, 12);
	tx_data[5] = float_to_uint(Kd, KD_MIN, KD_MAX, 12)>>4;
	tx_data[6] = float_to_uint(Kd, KD_MIN, KD_MAX, 12)<<4 | float_to_uint(Torque, T_MIN, T_MAX, 12)>>8;
	tx_data[7] = float_to_uint(Torque, T_MIN, T_MAX, 12);
	can_MIT_txd();
}

//MITλ��ģʽ
void RobStride_Motor_MIT_PositionControl(float position_rad, float speed_rad_per_s)
{
	mittxCanIdEx.id = (1 << 8) | CAN_ID;
	MITtxMsg.DLC = 8;
	memcpy(&tx_data[0], &position_rad, 4); 	//��λ�����ݸ��Ƶ���������������
	memcpy(&tx_data[4], &speed_rad_per_s, 4); 	//���ٶ����ݸ��Ƶ���������������
	can_MIT_txd();
}

//MIT�ٶ�ģʽ
void RobStride_Motor_MIT_SpeedControl(float speed_rad_per_s, float current_limit)
{
	mittxCanIdEx.id = (2 << 8) | CAN_ID;
	MITtxMsg.DLC = 8;
	memcpy(&tx_data[0], &speed_rad_per_s, 4); 	//��λ�����ݸ��Ƶ���������������
	memcpy(&tx_data[4], &current_limit, 4); 	//���ٶ����ݸ��Ƶ���������������
	can_MIT_txd();
}

//MIT�������
void RobStride_Motor_MIT_SetZeroPos(void)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = 0xFF;
	tx_data[7] = 0xFE;
	can_MIT_txd();
}

//MIT���ݱ��棨��Ҫ���µ����¹̼���
void RobStride_Motor_MIT_MotorDataSave(void)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = 0xFF;
	tx_data[7] = 0xF8;
	can_MIT_txd();
}

//MIT�����ϱ�����Ҫ���µ����¹̼���
void RobStride_Motor_MIT_ProtactiveEscalationSet(uint8_t F_CMD)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = 0xFF;
	tx_data[1] = 0xFF;
	tx_data[2] = 0xFF;
	tx_data[3] = 0xFF;
	tx_data[4] = 0xFF;
	tx_data[5] = 0xFF;
	tx_data[6] = F_CMD;//���� F_CMD �ֽ�Ϊ���Э���������� 0 Ϊ���ϱ���Ĭ�ϣ�1 Ϊ�ϱ�
	tx_data[7] = 0xF9;
	can_MIT_txd();
}

//MIT������
void Get_RobStride_Motor_MIT_parameter(uint16_t Index)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = Index;
	tx_data[1] = Index>>8;
	can_MIT_txd();
}

//MITд����
void Set_RobStride_Motor_MIT_parameter(uint16_t Index, float Value)
{
	mittxCanIdEx.id = CAN_ID;
	MITtxMsg.DLC = 8;
	tx_data[0] = Index;
	tx_data[1] = Index>>8;
	tx_data[2] = 0x00;
	tx_data[3] = 0x00;	
	memcpy(&tx_data[4],&Value,4);	
  can_MIT_txd();
}

/*******************************************************************************
* @����     	: ���մ�������		��ͨ������2 17Ӧ��֡ 0Ӧ��֡��
* @����1        : 
* @����ֵ 		: None
* @����  		: drwֻ��ͨ��ͨ��17�����Ժ����ֵ
*******************************************************************************/
void parse_motor_feedback(void)
{
	if (rxMsg.ExtId != 0)
	{
		uint8_t motor_id = rxCanIdEx.data & 0xFF;

		mf.uncalibrated        = ((rxCanIdEx.data >> 13) & 0x01) ? 1 : 0;
		mf.StallOverloadFault  = ((rxCanIdEx.data >> 12) & 0x01) ? 1 : 0;
		mf.MagneticEncoderFault= ((rxCanIdEx.data >> 11) & 0x01) ? 1 : 0;
		mf.OverTemperatureFault= ((rxCanIdEx.data >> 10) & 0x01) ? 1 : 0;
		mf.DriveFault          = ((rxCanIdEx.data >> 9)  & 0x01) ? 1 : 0;
		mf.UnderVoltageFault   = ((rxCanIdEx.data >> 8)  & 0x01) ? 1 : 0;

		uint8_t mode_tmp = (uint8_t)((rxCanIdEx.data >> 14) & 0x03);
		if (mode_tmp > running) mode_tmp = rest;   // ��Χ����
		mf.mms = (enum ModeStatus)mode_tmp;

		uint16_t angle_raw  = ((uint16_t)rx_data[0] << 8) | rx_data[1];
		uint16_t speed_raw  = ((uint16_t)rx_data[2] << 8) | rx_data[3];
		uint16_t torque_raw = ((uint16_t)rx_data[4] << 8) | rx_data[5];
		uint16_t temp_raw   = ((uint16_t)rx_data[6] << 8) | rx_data[7];

		txCanIdEx.id=motor_id;
		txCanIdEx.data=rxCanIdEx.id;

		mf.angle       = uint16_to_float(angle_raw, P_MIN, P_MAX);
		mf.speed       = uint16_to_float(speed_raw, V_MIN, V_MAX);
		mf.torque      = uint16_to_float(torque_raw, T_MIN, T_MAX);
		mf.temperature = temp_raw / 10.0f;
	}
	else if(rxMsg.StdId !=0)
	{
		uint16_t angle_raw  = ((uint16_t)rx_data[1] << 8) | rx_data[2];
		uint16_t speed_raw  = ((uint16_t)rx_data[3] << 8) | rx_data[4];
		uint16_t torque_raw = ((uint16_t)(rx_data[4]  << 4) << 8) | rx_data[5];
		uint16_t temp_raw   = ((uint16_t)rx_data[6] << 8) | rx_data[7];
		
		mf.angle       = uint16_to_float(angle_raw, P_MIN, P_MAX);
		mf.speed       = uint16_to_float(speed_raw, V_MIN, V_MAX);
		mf.torque      = uint16_to_float(torque_raw, T_MIN, T_MAX);
		mf.temperature = temp_raw / 10.0f;
	}
	
}


