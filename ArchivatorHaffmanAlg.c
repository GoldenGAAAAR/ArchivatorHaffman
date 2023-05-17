#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <locale.h>
#include <stdlib.h>
#include <io.h>

// ��������� ������������ ����� ��������
typedef struct HEAD
{
	char  magic[4];       // ������� Haff
	short n;              // ���������� ��������� ��������
	int   size_file;      // ����� ����� ����� ����������
} HEAD;

// ���� ������ ��������, ���������� ����� ������� ��������� �����
typedef struct NODE
{
	int           cnt;    // ���������� ���������� �������
	unsigned char c;      // ���������� ��� ������
	short         left;   // ���������� ������ ����������
	short         right;  // ���������� ������� ����������
	short         cur;    // ���������� ������ ����
	short         up;     // ���������� ��������
} NODE;

// ���� ������ �������� ��� ���������� ���������� (����� ����� > 255)
typedef struct NODE1
{
	short         left;   // ���������� ������ ����������
	short         right;  // ���������� ������� ����������
} NODE1;

// ���� ������ �������� ��� ���������� ���������� (����� ����� <= 255)
typedef struct NODE2
{
	unsigned char left;   // ���������� ������ ����������
	unsigned char right;  // ���������� ������� ����������
} NODE2;

// ����� ����� ������ ��������� �����
#define SIZE_BLOCK      1024

// ���������� ���������
HEAD head = { { 'H', 'a', 'f', 'f' } };

// �������� ����
FILE *f1;

// �������������� ����
FILE *f2;

// ��� ��������� �����
char *name1;

// ��� ��������������� �����
char *name2;

// ����� ��� ������ ��������� �����
unsigned char buf[SIZE_BLOCK];

// ����� ��� ����������� ����� ��������������� �����
unsigned char bit[SIZE_BLOCK];

// ����� ��������� ����� �� �����
int ind_c;

// ����� ������������� ����
int ind_bit;

// ����� ��������� ����� � ������
int size_file;

// ���� �� ����� ������ � �����
char code[256];

// ����� ���� �� ����� ������ � �����
int ind_code;

// ���������� � ������� ��������
NODE t1[256], t2[256 + 255];

// ������ ���������� �� ������ ������
NODE *p[256];

// ���������� ��� ����������
NODE1 t3[256 + 255];
NODE2 t4[256];

// ���������� ��������� �������� � �������� �����
int n;

// ��� ������ ����� ������������ �����
unsigned char Byte;

int i, offs, cur, left, right;

// ������ ���������� ����� ��������� �����
unsigned char get_byte(void)
{
	unsigned char v;

	// ��������� ��������� ���� ��������� �����?
	if (ind_c == 0)
	{
		fread(buf, SIZE_BLOCK, 1, f1);
	}

	v = buf[ind_c];
	if (++ind_c == SIZE_BLOCK)
		ind_c = 0;
	return v;
}

// ������ ���������� ���� � �������������� ����
void put_bit(int v)
{
	unsigned char *adr_byte;

	// ��������� ���� ����� ��������?
	if (ind_bit == SIZE_BLOCK * 8)
	{
		// �������� ���
		fwrite(bit, SIZE_BLOCK, 1, f2);
		ind_bit = 0;

		// ������� ����
		memset(bit, 0, SIZE_BLOCK);
	}

	// ��������� ����� ����� � �����, ���� ������������ ���
	adr_byte = bit + ind_bit / 8;

	// ������� ������������ ��� (���� �� �� �������) �� ��������������� �����
	// � ����������� ��� � ��������� �����
	if (v)
		adr_byte[0] |= (1 << (ind_bit % 8));

	// �������� ��� ���� ��� � ���� �����
	++ind_bit;
}

// ���������� ���������� ����� �����
void close_bit(void)
{
	fwrite(bit, (ind_bit + 7) / 8, 1, f2);
	fclose(f2);
}

// ������ ���������� ���� ������������ �����
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

// ��������
void coding(void)
{
	NODE *q;
	int  r;

	// ������� ������ ��������
	for (i = 0; i < size_file; ++i)
	{
		++t1[get_byte()].cnt;
	}

	fclose(f1);

	// ���� ��������� ���������, �������� ������ ��������.
	// ��������� �������� ����, ��� �� ���� ������ �� ����������
	// � ���������� ������.
	// ���� ���� �������� ��� ���������� ������� - ������� ��� � ��������� ������
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

	// ���������� ��������� ������������ �����
	head.n = (short)n;
	head.size_file = size_file;

	// �������� ��������� � ������ ������������ �����
	fwrite(&head, sizeof(head), 1, f2);

	// ���� �������� ������ ���������� �������?
	if (n == 1)
	{
		// �������� ������, �� �������� ������� ���������� �����
		fwrite(&t1[0].c, sizeof(unsigned char), 1, f2);

		// �������� ������� ������ ���������
		fclose(f2);
		return;
	}

	// ��������� ���������� ������
	for (;;)
	{
		// ��������� ��������� ������ ��������� ������
		ShellSort();

		// ����������� 1-� ������� �������
		if ((left = t1[offs].cur) < 0)
		{
			// ������� ��� �� ���������� � ���������� ������,
			// �������� ��� � ������, �������������� ����� ������
			t2[left = cur++] = t1[offs];
		}

		// ����������� 2-� ������� �������
		if ((right = t1[offs + 1].cur) < 0)
		{
			// ������� ��� �� ���������� � ���������� ������,
			// �������� ��� � ������, �������������� ����� ������
			t2[right = cur++] = t1[offs + 1];
		}

		// ���������� �����������
		t2[cur].cur = t2[left].up = cur;
		t2[right].up = -cur;
		t2[cur].left = left;
		t2[cur].right = right;

		// ������������� ������� �������� ������ ���� � ���� ����� �����������
		t2[cur].cnt = t1[offs].cnt + t1[offs + 1].cnt;

		++cur;

		// ������ ���������?
		if (--n == 1)
			break;

		// �������� � ����������� ������ ���������� ��������
		t1[++offs] = t2[cur - 1];
	}

	// ��������� ������ ������ �� ������ ������
	// � ��������� ������ ��� ����������
	q = t2;
	for (i = 0; i < cur; ++i, ++q)
	{
		// ��� ����?
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

	// �������� ������ ������ ��� ���������� � ���� ������
	// � ����������� �� ���������� ��������� ������
	// ����������� left � right ����� ���� 8 ��� 16 ���
	fwrite(&cur, sizeof(cur), 1, f2);
	if (cur <= 255)
		fwrite(t4, sizeof(NODE2), cur, f2);
	else
		fwrite(t3, sizeof(NODE1), cur, f2);

	// ������ ��������� �������� ���� ��� ������������� ��������
	f1 = fopen(name1, "rb");

	// ������������ ��� �����
	ind_c = 0;
	for (i = 0; i < size_file; ++i)
	{
		// ��������� �� ���� ������, ��������������� �������� ������������ �����
		q = p[get_byte()];

		// ��������� ����������� � ����� ������ � ������������� ���������
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

		// �������� � �������������� ���� ���� ����������� ����
		// � �������� �������
		while (--ind_code >= 0)
		{
			put_bit(code[ind_code]);
		}
	}

	// ������� ����� ����� ����������
	close_bit();

	// ������� �������� ����
	fclose(f1);
}

// ����������
void encoding(void)
{
	unsigned char c;
	NODE1         *q;
	NODE2         *q2;

	// ����� ����� ������ ���� ����������� ��� ������ ���� ������
	if ((unsigned)size_file < sizeof(head) + 1)
		goto err;

	fread(&head, sizeof(head), 1, f1);
	if (memcmp(head.magic, "Haff", 4) || head.n == 0 || head.size_file == 0)
	{

	err:

		fclose(f1);
		fclose(f2);

		printf("��������� ������ ������������ �����\n");
		return;
	}

	// ���������� ��������� �������� ��������� �����
	n = head.n;

	// ������ ������������� ��������� �����
	size_file = head.size_file;

	// ������������ ���� �������� ������ ���������� �������?
	if (n == 1)
	{
		// ������, �� �������� ������� ���������� ������������� �����
		fread(&c, sizeof(unsigned char), 1, f1);

		// ����������� ������������ ����
		while (size_file--)
			fwrite(&c, sizeof(unsigned char), 1, f2);
	}
	else
	{
		// ������ ������ ��� ���������� � ���� ������
		fread(&cur, sizeof(cur), 1, f1);

		// � ����������� �� ���������� ��������� ������
		// ����������� left � right ����� ���� 8 ��� 16 ���
		if (cur <= 255)
		{
			fread(t4, sizeof(NODE2), cur, f1);

			// ������������� ���� ����
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

			// ������������� ���� ����
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

// ����� �������� �������� � ������
char *del_blank(char *s)
{
	char *p;

	// ���������� ��������� ������ �������
	while (isspace(*s))
		++s;

	// ������ ������?
	if (*s == 0)
	{
		printf("��� ����� �� ������\n");
		return NULL;
	}

	// ���������� ��������� ������ �������
	for (p = s + strlen(s); isspace(*--p);)
		;
	p[1] = 0;
	return s;
}

int main()
{
	char src[1024], dest[1024];
	int  n, size;

	// ������� ������ ��� ������ �� ������� ������������� ��������
	setlocale(LC_ALL, "Russian");

	printf("�������: 0 - ���������� ������, 1 - ��������, 2 - ����������: ");
	gets(src);

	switch (n = atoi(src))
	{

	default:
		printf("�������� ��������\n");

	case 0:
		return 0;

	case 1: case 2:
		printf("�������� ����      : ");
		gets(src);
		if ((name1 = del_blank(src)) == NULL)
			break;

		printf("�������������� ����: ");
		gets(dest);
		if ((name2 = del_blank(dest)) == NULL)
			break;

		if (strcmp(name1, name2) == 0)
		{
			printf("���������� ���� ������\n");
			break;
		}
	}

	// ������ �� �������� ����?
	if ((f1 = fopen(name1, "rb")) == NULL)
	{
		printf("�� ������ �������� ����\n");
		return 0;
	}

	// ����� �� ������� ���������?
	if ((f2 = fopen(name2, "wb")) == NULL)
	{
		fclose(f1);
		printf("������ ������� �������������� ����\n");
		return 0;
	}

	// ����� ��������� ����� � ������
	size_file = size = filelength(fileno(f1));

	// ������ ������, ����� ����� �������
	if (size_file == 0)
	{
		fclose(f1);
		fclose(f2);
		printf("���� ������\n");
		return 0;
	}

	// ������������� ������
	ind_c = ind_bit = ind_code = 0;

	// ��������� ������ ��� ����������
	if (n == 1)
		coding();
	else
		encoding();

	f1 = fopen(name2, "rb");
	printf("������ ��������� ����� %d, ������ ��������������� ����� %d\n", size, (int)filelength(fileno(f1)));
	fclose(f1);

	return 0;
}

