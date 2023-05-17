#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <locale.h>
#include <stdlib.h>
#include <io.h>

// Заголовок упакованного файла Хаффмана
typedef struct HEAD
{
	char  magic[4];       // Символы Haff
	short n;              // Количество различных символов
	int   size_file;      // Длина файла после распаковки
} HEAD;

// Узел дерева Хаффмана, созданного после анализа исходного файла
typedef struct NODE
{
	int           cnt;    // Количество повторений символа
	unsigned char c;      // Собственно сам символ
	short         left;   // Дескриптор левого наследника
	short         right;  // Дескриптор правого наследника
	short         cur;    // Дескриптор самого узла
	short         up;     // Дескриптор родителя
} NODE;

// Узел дерева Хаффмана для дальнейшей распаковки (когда узлов > 255)
typedef struct NODE1
{
	short         left;   // Дескриптор левого наследника
	short         right;  // Дескриптор правого наследника
} NODE1;

// Узел дерева Хаффмана для дальнейшей распаковки (когда узлов <= 255)
typedef struct NODE2
{
	unsigned char left;   // Дескриптор левого наследника
	unsigned char right;  // Дескриптор правого наследника
} NODE2;

// Длина блока чтения исходного файла
#define SIZE_BLOCK      1024

// Магический заголовок
HEAD head = { { 'H', 'a', 'f', 'f' } };

// Исходный файл
FILE *f1;

// Результирующий файл
FILE *f2;

// Имя исходного файла
char *name1;

// Имя результирующего файла
char *name2;

// Буфер для чтения исходного файла
unsigned char buf[SIZE_BLOCK];

// Буфер для упакованных битов результирующего файла
unsigned char bit[SIZE_BLOCK];

// Номер читаемого байта из файла
int ind_c;

// Номер записываемого бита
int ind_bit;

// Длина исходного файла в байтах
int size_file;

// Путь от листа дерева к корню
char code[256];

// Длина пути от листа дерева к корню
int ind_code;

// Информация о частоте символов
NODE t1[256], t2[256 + 255];

// Массив указателей на листья дерева
NODE *p[256];

// Информация для распаковки
NODE1 t3[256 + 255];
NODE2 t4[256];

// Количество различных символов в исходном файле
int n;

// Для чтения битов упакованного файла
unsigned char Byte;

int i, offs, cur, left, right;

// Чтение очередного байта исходного файла
unsigned char get_byte(void)
{
	unsigned char v;

	// Прочитать очередной блок исходного файла?
	if (ind_c == 0)
	{
		fread(buf, SIZE_BLOCK, 1, f1);
	}

	v = buf[ind_c];
	if (++ind_c == SIZE_BLOCK)
		ind_c = 0;
	return v;
}

// Запись очередного бита в результирующий файл
void put_bit(int v)
{
	unsigned char *adr_byte;

	// Очередной блок битов заполнен?
	if (ind_bit == SIZE_BLOCK * 8)
	{
		// Отправим его
		fwrite(bit, SIZE_BLOCK, 1, f2);
		ind_bit = 0;

		// Обнулим блок
		memset(bit, 0, SIZE_BLOCK);
	}

	// Определим номер байта в блоке, куда записывается бит
	adr_byte = bit + ind_bit / 8;

	// Сдвинем записываемый бит (если он не нулевой) на соответствующее место
	// и присоединим его к остальным битам
	if (v)
		adr_byte[0] |= (1 << (ind_bit % 8));

	// Записали еще один бит в блок битов
	++ind_bit;
}

// Сохранение последнего блока битов
void close_bit(void)
{
	fwrite(bit, (ind_bit + 7) / 8, 1, f2);
	fclose(f2);
}

// Чтение очередного бита упакованного файла
int get_bit(void)
{
	int v;

	if (ind_bit == 0)
	{
		fread(&Byte, sizeof(unsigned char), 1, f1);
	}

	v = (Byte >> ind_bit) & 1;
	if (++ind_bit == 8)
		ind_bit = 0;
	return v;
}

void ShellSort(void)
{
	int  gap, i, j, n1, n2;
	NODE temp;

	for (gap = n / 2; gap > 0; gap /= 2)
	{
		for (i = gap; i < n; ++i)
		{
			for (j = i - gap; j >= 0; j -= gap)
			{
				n1 = j + offs;
				n2 = n1 + gap;
				if (t1[n1].cnt < t1[n2].cnt)
					break;
				temp = t1[n1];
				t1[n1] = t1[n2];
				t1[n2] = temp;
			}
		}
	}
}

// Упаковка
void coding(void)
{
	NODE *q;
	int  r;

	// Подсчет частот символов
	for (i = 0; i < size_file; ++i)
	{
		++t1[get_byte()].cnt;
	}

	fclose(f1);

	// Файл полностью обработан, уплотним массив символов.
	// Установим признаки того, что ни один символ не участвовал
	// в построении дерева.
	// Если файл содержит все одинаковые символы - выделим это в отдельный случай
	for (i = n = 0; i <= 255; ++i)
	{
		if (t1[i].cnt)
		{
			t1[n].cur = -1;
			t1[n].cnt = t1[i].cnt;
			t1[n].c = i;
			++n;
		}
	}

	// Сформируем заголовок упакованного файла
	head.n = (short)n;
	head.size_file = size_file;

	// Сохраним заголовок в начале упакованного файла
	fwrite(&head, sizeof(head), 1, f2);

	// Файл содержит только одинаковые символы?
	if (n == 1)
	{
		// Сохраним символ, из которого состоит содержимое файла
		fwrite(&t1[0].c, sizeof(unsigned char), 1, f2);

		// Упаковка особого случая закончена
		fclose(f2);
		return;
	}

	// Выполняем построение дерева
	for (;;)
	{
		// Сортируем очередной массив элементов дерева
		ShellSort();

		// Анализируем 1-й младший элемент
		if ((left = t1[offs].cur) < 0)
		{
			// Элемент еще не участвовал в построении дерева,
			// отправим его в массив, представляющий собой дерево
			t2[left = cur++] = t1[offs];
		}

		// Анализируем 2-й младший элемент
		if ((right = t1[offs + 1].cur) < 0)
		{
			// Элемент еще не участвовал в построении дерева,
			// отправим его в массив, представляющий собой дерево
			t2[right = cur++] = t1[offs + 1];
		}

		// Сформируем дескрипторы
		t2[cur].cur = t2[left].up = cur;
		t2[right].up = -cur;
		t2[cur].left = left;
		t2[cur].right = right;

		// Устанавливаем счетчик символов нового узла в виде суммы наследников
		t2[cur].cnt = t1[offs].cnt + t1[offs + 1].cnt;

		++cur;

		// Дерево построено?
		if (--n == 1)
			break;

		// Поместим в сортируемый массив очередного родителя
		t1[++offs] = t2[cur - 1];
	}

	// Формируем массив ссылок на листья дерева
	// и сохраняем дерево для распаковки
	q = t2;
	for (i = 0; i < cur; ++i, ++q)
	{
		// Это лист?
		if (q->cur < 0)
		{
			if (cur <= 255)
				t4[i].left = t4[i].right = q->c;
			else
				t3[i].left = t3[i].right = q->c;
			p[q->c] = q;
		}
		else
		{
			if (cur <= 255)
			{
				t4[i].left = (unsigned char)q->left;
				t4[i].right = (unsigned char)q->right;
			}
			else
			{
				t3[i].left = q->left;
				t3[i].right = q->right;
			}
		}
	}

	// Сохраним размер дерева для распаковки и само дерево
	// В зависимости от количества элементов дерева
	// дескрипторы left и right могут быть 8 или 16 бит
	fwrite(&cur, sizeof(cur), 1, f2);
	if (cur <= 255)
		fwrite(t4, sizeof(NODE2), cur, f2);
	else
		fwrite(t3, sizeof(NODE1), cur, f2);

	// Заново открываем исходный файл для окончательной упаковки
	f1 = fopen(name1, "rb");

	// Перекодируем все байты
	ind_c = 0;
	for (i = 0; i < size_file; ++i)
	{
		// Указатель на лист дерева, соответствующий текущему прочитанному байту
		q = p[get_byte()];

		// Выполняем восхождение к корню дерева с формированием кодировки
		for (ind_code = 0; (r = q->up) != 0; q = t2 + r)
		{
			if (r < 0)
			{
				r = -r;
				code[ind_code] = 1;
			}
			else
			{
				code[ind_code] = 0;
			}
			++ind_code;
		}

		// Отправим в результирующий файл биты пройденного пути
		// в обратном порядке
		while (--ind_code >= 0)
		{
			put_bit(code[ind_code]);
		}
	}

	// Закроем поток битов результата
	close_bit();

	// Закроем исходный файл
	fclose(f1);
}

// Распаковка
void encoding(void)
{
	unsigned char c;
	NODE1         *q;
	NODE2         *q2;

	// Длина файла должна быть достаточной для нашего типа файлов
	if ((unsigned)size_file < sizeof(head) + 1)
		goto err;

	fread(&head, sizeof(head), 1, f1);
	if (memcmp(head.magic, "Haff", 4) || head.n == 0 || head.size_file == 0)
	{

	err:

		fclose(f1);
		fclose(f2);

		printf("Ошибочный формат упакованного файла\n");
		return;
	}

	// Количество различных символов исходного файла
	n = head.n;

	// Размер оригинального исходного файла
	size_file = head.size_file;

	// Оригинальный файл содержал только одинаковые символы?
	if (n == 1)
	{
		// Символ, из которого состоит содержимое оригинального файла
		fread(&c, sizeof(unsigned char), 1, f1);

		// Воссоздадим оригинальный файл
		while (size_file--)
			fwrite(&c, sizeof(unsigned char), 1, f2);
	}
	else
	{
		// Размер дерева для распаковки и само дерево
		fread(&cur, sizeof(cur), 1, f1);

		// В зависимости от количества элементов дерева
		// дескрипторы left и right могут быть 8 или 16 бит
		if (cur <= 255)
		{
			fread(t4, sizeof(NODE2), cur, f1);

			// Распаковываем весь файл
			while (size_file--)
			{
				for (q2 = t4 + cur - 1; q2->left != q2->right;)
				{
					if (get_bit())
						q2 = t4 + q2->right;
					else
						q2 = t4 + q2->left;
				}
				fwrite(&q2->left, sizeof(unsigned char), 1, f2);
			}
		}
		else
		{
			fread(t3, sizeof(NODE1), cur, f1);

			// Распаковываем весь файл
			while (size_file--)
			{
				for (q = t3 + cur - 1; q->left != q->right;)
				{
					if (get_bit())
						q = t3 + q->right;
					else
						q = t3 + q->left;
				}
				fwrite(&q->left, sizeof(unsigned char), 1, f2);
			}
		}
	}
	fclose(f1);
	fclose(f2);
}

// Сброс концевых пробелов в строке
char *del_blank(char *s)
{
	char *p;

	// Игнорируем начальные пустые символы
	while (isspace(*s))
		++s;

	// Пустая строка?
	if (*s == 0)
	{
		printf("Имя файла не задано\n");
		return NULL;
	}

	// Игнорируем хвостовые пустые символы
	for (p = s + strlen(s); isspace(*--p);)
		;
	p[1] = 0;
	return s;
}

int main()
{
	char src[1024], dest[1024];
	int  n, size;

	// Задание режима для вывода на консоль кириллических символов
	setlocale(LC_ALL, "Russian");

	printf("Введите: 0 - завершение работы, 1 - упаковка, 2 - распаковка: ");
	gets(src);

	switch (n = atoi(src))
	{

	default:
		printf("Параметр ошибочен\n");

	case 0:
		return 0;

	case 1: case 2:
		printf("Исходный файл      : ");
		gets(src);
		if ((name1 = del_blank(src)) == NULL)
			break;

		printf("Результирующий файл: ");
		gets(dest);
		if ((name2 = del_blank(dest)) == NULL)
			break;

		if (strcmp(name1, name2) == 0)
		{
			printf("Совпадение имен файлов\n");
			break;
		}
	}

	// Найден ли исходный файл?
	if ((f1 = fopen(name1, "rb")) == NULL)
	{
		printf("Не найден исходный файл\n");
		return 0;
	}

	// Можно ли создать результат?
	if ((f2 = fopen(name2, "wb")) == NULL)
	{
		fclose(f1);
		printf("Нельзя создать результирующий файл\n");
		return 0;
	}

	// Длина исходного файла в байтах
	size_file = size = filelength(fileno(f1));

	// Особый случай, когда длина нулевая
	if (size_file == 0)
	{
		fclose(f1);
		fclose(f2);
		printf("Файл пустой\n");
		return 0;
	}

	// Инициализация данных
	ind_c = ind_bit = ind_code = 0;

	// Выполняем сжатие или распаковку
	if (n == 1)
		coding();
	else
		encoding();

	f1 = fopen(name2, "rb");
	printf("Размер исходного файла %d, размер результирующего файла %d\n", size, (int)filelength(fileno(f1)));
	fclose(f1);

	return 0;
}

