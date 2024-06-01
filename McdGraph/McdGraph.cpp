#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <graphics.h>
#include <easyx.h>
#include <string>


#define MAX_SECONDS 100000 // 系统运行的最大秒数，即 7:00 到 22:00 共计 15 小时
#define MAX_FOODS 101     // 最大食物种类数
#define MAX_COMBOS 101    // 最大套餐种类数
#define MAX_COMBO_FOODS 6    // 套餐最多食物种数

bool isValid = true;   //系统是否打开
int current_time = 0;   //系统运行到第多少秒
int front;		//队列头
int rear;		//队列尾
int num_wait = 0;//当前有多少个订单正在等待
int W1;
int W2;
// 食物结构体
typedef struct {
	char name[51];  // 食物名称
	int prep_time;  // 制作时间
	int capacity;   // 最大存储容量
	int remaining;  // 剩余存储量
	int last_completion_time;  // 上一次完成的时间
} Food;

// 套餐结构体
typedef struct {
	char name[51];          // 套餐名称
	int num_foods;          // 包含的食物种类数
	int foods[MAX_COMBO_FOODS];
} Combo;

typedef struct {
	int time;              // 订单时间
	int food_index;         //食物在食物数组里的索引 如果是套餐则为-1
	int combo_index;        //食物在套餐数组里的索引 如果是食物则为-1
	bool is_combo;          // 是否是套餐订单
	int state;              //订单状态 0 代表下单不成功 1代表已经完成 2代表正在等待
	int completion_time;   // 订单完成时间
	int food_state[MAX_COMBO_FOODS];    //每种食物的准备状态
} Order;

// 菜单文件结构体
typedef struct {
	int num_foods;     // 食物种类数
	int num_combos;          // 套餐种类数
	Food foods[MAX_FOODS];   //食物数组
	Combo combos[MAX_COMBOS];  // 套餐数组
} Menu;

//主要函数声明
void read_menu_file(Menu* menu, const char* filename);
void printTime(Order* order);
void produceFood(Food* food);
void takeNewOrder(Order* order, Menu* menu, int* waitlist, int* num_wait, int num_orders, int front);
void takeWaitOrder(Order* orders, Menu* menu, int* waitlist, int* num_wait);
void upDateSystem();
int timeTrans(int hour, int min, int second);

//easyX相关函数声明
void initGUI();//初始化GUI
void drawInterface(Menu* menu);//绘制界面
void charToTChar(const char* charArray, TCHAR* tcharArray);



int main() {
	Menu menu;
	const char filename[] = "dict.dic";
	// 读取菜单文件
	read_menu_file(&menu, filename);
	initGUI();
	drawInterface(&menu);
	// 读取订单
	int num_orders;
	scanf("%d", &num_orders);
	Order* orders = (Order*)malloc((num_orders) * sizeof(Order));
	int* waitlist = (int*)malloc((W1) * sizeof(int));
	for (int i = 0; i < num_orders; i++)
	{
		int hour;
		int min;
		int sec;
		scanf("%d:%d:%d", &hour, &min, &sec);
		orders[i].time = timeTrans(hour, min, sec);
		char name[51];
		scanf("%s", name);
		bool isfood = false;
		orders[i].food_index = -1;
		orders[i].combo_index = -1;
		for (int j = 0; j < menu.num_foods; j++)
		{
			if (strcmp(name, menu.foods[j].name) == 0) {
				orders[i].food_index = j;
				orders[i].is_combo = false;
				isfood = true;
				break;
			}
		}
		if (!isfood) {
			for (int k = 0; k < menu.num_combos; k++)
			{
				if (strcmp(name, menu.combos[k].name) == 0) {
					orders[i].combo_index = k;
					orders[i].is_combo = true;
					for (int p = 0; p < menu.combos[orders[i].combo_index].num_foods; p++)
					{
						//初始化使订单食物状态都是0
						orders[i].food_state[p] = 0;
					}
					break;
				}
			}
		}
	}
	//初始化队列
	front = 0;//队列头
	rear = num_orders - 1;//队列尾
	//系统正式开始运行
	while (current_time < MAX_SECONDS)
	{
		for (int p = 0; p < menu.num_foods; p++)
		{
			produceFood(&menu.foods[p]);
		}
		if (num_wait != 0)
		{
			takeWaitOrder(orders, &menu, waitlist, &num_wait);
		}
		//该秒没有订单，直接跳过
		if (orders[front].time == current_time)
		{
			//这一秒系统关闭，订单下单失败。
			if (!isValid)
			{
				orders[front].state = 0;
				//printTime(&orders[front]);//输出Fail
				front++;
			}
			else
			{
				takeNewOrder(&orders[front], &menu, waitlist, &num_wait, num_orders, front);
				front++;
				upDateSystem();
			}

		}
		upDateSystem();
		current_time++;
		if (front == rear + 1 && num_wait == 0)
		{
			for (int h = 0; h < num_orders; h++)
			{
				printTime(&orders[h]);
			}
			return 0;
		}
	}
	return 0;
}

// 读取菜单文件并初始化菜单数据结构
void read_menu_file(Menu* menu, const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("Error opening file.\n");
		exit(1);
	}

	// 读取食物种类数和套餐种类数
	fscanf(file, "%d %d", &menu->num_foods, &menu->num_combos);

	// 读取食物信息
	for (int i = 0; i < menu->num_foods; i++) {
		fscanf(file, "%s", menu->foods[i].name);
		menu->foods[i].remaining = 0;
		menu->foods[i].last_completion_time = 0;
	}
	for (int i = 0; i < menu->num_foods; i++) {
		fscanf(file, "%d", &menu->foods[i].prep_time);
	}
	for (int i = 0; i < menu->num_foods; i++) {
		fscanf(file, "%d", &menu->foods[i].capacity);
	}

	// 读取未完成订单数量阈值
	fscanf(file, "%d %d", &W1, &W2);
	//读取套餐
	int i = 0;
	char name[51];
	while (fscanf(file, "%s", name) != EOF) {
		bool isFood = false;
		int t = -1;
		for (int j = 0; j < menu->num_foods; j++) {
			if (strcmp(name, menu->foods[j].name) == 0) {
				isFood = true;
				t = j;
				break;
			}
		}
		if (!isFood) { // 是套餐
			strcpy_s(menu->combos[i].name, name);
			menu->combos[i].num_foods = 0; // 初始化食物数量为0
			i++; // 更新套餐索引
		}
		else { // 是食物
			int k = menu->combos[i - 1].num_foods; // 获取当前套餐的食物数量
			if (k < MAX_COMBO_FOODS) { // 检查是否超出套餐最大食物种数
				menu->combos[i - 1].foods[k] = t; // 存储食物到当前套餐的食物数组中
				menu->combos[i - 1].num_foods++; // 更新当前套餐的食物数量
			}
		}
	}
	fclose(file);
}

void takeNewOrder(Order* order, Menu* menu, int* waitlist, int* num_wait, int num_orders, int front) {
	if (!order->is_combo) { // 是食物
		int re = menu->foods[order->food_index].remaining;
		if (re >= 1) { // 该种食物有余量，直接出餐
			menu->foods[order->food_index].remaining--;
			order->state = 1;
			order->completion_time = current_time;
		}
		else {
			waitlist[*num_wait] = front; // 将订单在订单数组中的索引加入等待队列
			(*num_wait)++; // 等待订单数自增
		}
	}
	else { // 是套餐
		int num_ready = 0; // 记录套餐中已准备好的食物数量
		for (int i = 0; i < menu->combos[order->combo_index].num_foods; i++) {
			int food_index = menu->combos[order->combo_index].foods[i]; // 获取食物在菜单食物数组中的索引
			if (order->food_state[i] == 1) { // 这种食物已经分配好
				num_ready++;
			}
			else {
				if (menu->foods[food_index].remaining >= 1) { // 没有分配且有余量
					order->food_state[i] = 1;
					menu->foods[food_index].remaining--;
					num_ready++; // 分配了一个食物，已准备好的数量加一
				}
			}
		}
		if (num_ready == menu->combos[order->combo_index].num_foods) { // 所有食物都准备好了
			order->state = 1; // 订单状态设为已完成
			order->completion_time = current_time;
		}
		else {
			order->state = 2; // 还有食物未准备好，订单状态设为等待
			waitlist[*num_wait] = front; // 将订单在订单数组中的索引加入等待队列
			(*num_wait)++; // 等待订单数自增
		}
	}
}

void takeWaitOrder(Order* orders, Menu* menu, int* waitlist, int* num_wait) {
	if (*num_wait > 0)
	{
		for (int i = 0; i < *num_wait; i++) {
			Order* current_order = &orders[waitlist[i]];
			// 如果订单是食物订单
			if (!current_order->is_combo) {
				if (menu->foods[current_order->food_index].remaining > 0) {
					// 如果该食物有余量，直接出餐
					menu->foods[current_order->food_index].remaining--;
					current_order->state = 1; // 订单状态设为已完成
					current_order->completion_time = current_time;
					//printTime(current_order); // 打印出餐时间
					// 将当前订单从等待队列中删除
					for (int j = i; j < *num_wait - 1; j++) {
						waitlist[j] = waitlist[j + 1];
					}
					(*num_wait)--; // 更新等待订单数量
					i--; // 回退一步，重新检查当前位置的订单
					continue; // 继续处理下一个订单
				}
			}
			else { // 是套餐订单
				bool all_ready = true; // 标记当前套餐订单的所有食物是否都已准备好
				// 检查当前套餐订单的所有食物状态
				for (int j = 0; j < menu->combos[current_order->combo_index].num_foods; j++) {
					int food_index = menu->combos[current_order->combo_index].foods[j]; // 获取食物在菜单食物数组中的索引
					if (current_order->food_state[j] == 0) {
						all_ready = false;
						if (menu->foods[food_index].remaining > 0) {
							// 如果该食物有余量且未分配，尽可能分配食物
							current_order->food_state[j] = 1;
							menu->foods[food_index].remaining--;
						}
					}
				}
				all_ready = true;
				for (int k = 0; k < menu->combos[current_order->combo_index].num_foods; k++)
				{
					int food_index = menu->combos[current_order->combo_index].foods[k]; // 获取食物在菜单食物数组中的索引
					if (current_order->food_state[k] == 0) {
						all_ready = false;
					}
				}
				// 如果所有食物都准备好了
				if (all_ready) {
					current_order->state = 1; // 订单状态设为已完成
					current_order->completion_time = current_time;
					// 将当前订单从等待队列中删除
					for (int j = i; j < *num_wait - 1; j++) {
						waitlist[j] = waitlist[j + 1];
					}
					(*num_wait)--; // 更新等待订单数量
					i--; // 回退一步，重新检查当前位置的订单
					continue; // 继续处理下一个订单
				}
			}
		}
	}
}

//HH:MM:SS -> int 格式 秒数；
int timeTrans(int hour, int min, int second) {
	return (hour - 7) * 3600 + min * 60 + second;
}

//将int格式的秒数用HH:MM:SS方式打印
void printTime(Order* order) {
	if (order->state == 0)//下单失败
	{
		printf("Fail\n");
	}
	else
	{
		int time = order->completion_time;
		int hours = time / 3600;
		int remainder = time % 3600;
		int minutes = remainder / 60;
		int secs = remainder % 60;
		printf("%02d:%02d:%02d\n", hours + 7, minutes, secs);
	}
}
//生产食物
void produceFood(Food* food) {
	if (food->capacity > food->remaining)
	{
		if (food->last_completion_time >= food->prep_time)
		{
			food->last_completion_time = 1;
			food->remaining++;
		}
		else
		{
			food->last_completion_time++;
		}
	}
}
//更新系统状态
void upDateSystem() {
	if (num_wait > W1) {
		isValid = false;
	}
	else if (num_wait < W2) {
		isValid = true;
	}
}

void initGUI() {
	initgraph(800, 600);  // Initialize a 800x600 window
	setbkcolor(WHITE);
	cleardevice();
	settextstyle(20, 0, _T("Arial"));  // Set text style for display
}

void charToTChar(const char* charArray, TCHAR* tcharArray) {
#ifdef UNICODE  // If project is compiled in Unicode mode
	mbstowcs(tcharArray, charArray, 256);  // Convert multi-byte to wide-char
#else
	strcpy(tcharArray, charArray);  // If ANSI mode, direct copy is sufficient
#endif
}

void drawInterface(Menu* menu) {
	// Clear previous content
	cleardevice();
	TCHAR tcharName[256];
	// Draw menu items as buttons
	for (int i = 0; i < menu->num_foods; i++) {
		rectangle(10, 50 * i + 10, 150, 50 * i + 60);
		charToTChar(menu->foods[i].name, tcharName);
		outtextxy(20, 50 * i + 20, tcharName);
	}

	// Additional UI elements like order queue and time display
	outtextxy(300, 10, _T("Current Orders:"));
	outtextxy(500, 10, _T("Current Time:"));
}