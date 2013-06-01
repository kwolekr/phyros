/*-
 * Copyright (c) 2009 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * treegraph.c - 
 *    Graphical Win32 utility to generate a balanced binary tree of string hashes, then
 *    output the structure as a static structure array in C.
 */

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define szAppName "treegraph"
#define inputfile  "commands.txt"
#define outputfile "cmdtree.c"

#define HEADINGSTR "typedef struct _node {\n\t" \
	"unsigned int key;\n\t" \
	"int data;\n\t" \
	"struct _node *lchild;\n\t" \
	"struct _node *rchild;\n" \
	"} NODE, *LPNODE;\n"

typedef struct _node {
	unsigned int key;
	int data;
	int index;
	struct _node *lchild;
	struct _node *rchild;
} NODE, *LPNODE;

NODE *root;

int index;
LPNODE *nodeary;

HWND hWnd_main;
HINSTANCE g_hInst;

int numnodes;
int treeheight;


///////////////////////////////////////////////////////////////////////////////


unsigned int hash(unsigned char *key, int arraylen) {
    unsigned int hash = 0;
    unsigned int i;
    for (i = 0; i < strlen((const char *)key); i++) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash & arraylen;
}


void ItemizeNode(LPNODE lpnode) {
	lpnode->index = index;
	nodeary[index] = lpnode;
	index++;
	if (lpnode->lchild)
		ItemizeNode(lpnode->lchild);
	if (lpnode->rchild)
		ItemizeNode(lpnode->rchild);
}


void PrintTreeCode(LPNODE lpnode, FILE *file) {
	char asdf[64];

	if (!lpnode)
		return;	
	
	for (int i = 0; i != numnodes; i++) {
		lpnode = nodeary[i];
		sprintf(asdf, "\t{0x%08x, %d, %s, %s},\n",
			lpnode->key, lpnode->data,
			lpnode->lchild ? "&nodes[%d]" : "0",
			lpnode->rchild ? "&nodes[%d]" : "0");
		fprintf(file, asdf,
			lpnode->lchild ? lpnode->lchild->index : 0,
			lpnode->rchild ? lpnode->rchild->index : 0);
	}
}


void GenerateCode(LPNODE lpnode) {
	FILE *file;
	
	nodeary = (LPNODE *)malloc(numnodes * sizeof(LPNODE));
	ItemizeNode(root);
	file = fopen(outputfile, "w");
	fprintf(file, HEADINGSTR);
	fprintf(file, "\nLPNODE treeroot = &nodes[%d];\n\nNODE nodes[] = {\n", root->data);
	PrintTreeCode(lpnode, file);
	fseek(file, ftell(file) - 3, SEEK_SET);
	fprintf(file, "\n};\n");
	fclose(file);
}


int GetHeight(LPNODE lpnode) {
	if (!lpnode)
		return 0;
	int hleft  = GetHeight(lpnode->lchild);
	int hright = GetHeight(lpnode->rchild);
	return ((hleft > hright) ? hleft : hright) + 1;
}


int RotateNodeLeft(LPNODE lpnode) {
	LPNODE p;
	if (!lpnode || !lpnode->rchild)
		return 0;

	p = lpnode->rchild;
	lpnode->rchild = p->rchild;

	p->rchild = p->lchild;
	p->lchild = lpnode->lchild;

	lpnode->lchild = p;

	unsigned int key = lpnode->key;
	int data = lpnode->data;
	lpnode->key = p->key;
	lpnode->data = p->data;
	p->key = key;
	p->data = data;
	return 1;
}


int RotateNodeRight(LPNODE lpnode) {
	LPNODE p;
	if (!lpnode || !lpnode->lchild)
		return 0;
	p = lpnode->lchild;
	lpnode->lchild = p->lchild;

	p->lchild = p->rchild;
	p->rchild = lpnode->rchild;
	lpnode->rchild = p;

	unsigned int key = lpnode->key;
	int data = lpnode->data;
	lpnode->data = p->data;
	lpnode->key  = p->key;
	p->data = data;
	p->key  = key;

	return 1;
}


void DSWBalanceTree(LPNODE lpnode) {
	LPNODE p = lpnode;
	int nc = 0;
	while (p) {	//tree to vine
		while (RotateNodeRight(p) == 1);
		p = p->rchild;
		nc++;
	}
	for (int i = nc >> 1; i; i >>= 1) {
		p = lpnode;	//vine to tree
		for (int i2 = 0; i2 < i; i2++) {		
			RotateNodeLeft(p);
			p = p->rchild;
		}
	}
}


void DrawNode(HDC hdc, int x, int y, int depth, LPNODE lpnode) {
	char asdf[32];
	RECT rc;

	rc.left = x;
	rc.top  = y;
	rc.right = x + 64;
	rc.bottom = y + 32;

	FrameRect(hdc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));
	sprintf(asdf, "0x%08x\n%d", lpnode->key, lpnode->index);
	DrawText(hdc, asdf, -1, &rc, DT_CENTER);

	if (lpnode->lchild)	{
		DrawNode(hdc, x - (300 / depth), y + 64, depth + 1, lpnode->lchild);
		MoveToEx(hdc, x + 32, y + 32, NULL);		
		LineTo(hdc, x - (300 / depth) + 32, y + 64);
	}

	if (lpnode->rchild)	{
		DrawNode(hdc, x + (300 / depth), y + 64, depth + 1, lpnode->rchild);
		MoveToEx(hdc, x + 32, y + 32, NULL);
		LineTo(hdc, x + 64 + (300 / depth) - 32, y + 64);
	}
}


void DoPainting(HWND hwnd) {
	char asdf[32];
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	HGDIOBJ hOld  = SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
	HGDIOBJ hOld2 =	SelectObject(ps.hdc, GetStockObject(DEFAULT_GUI_FONT));
	SetTextColor(hdc, 0xFFFFFF);
	SetBkMode(ps.hdc, TRANSPARENT);

	DrawNode(ps.hdc, (ps.rcPaint.right - ps.rcPaint.left) >> 1, 3, 1, root);

	sprintf(asdf, "nodes: %d, height: %d", numnodes, treeheight);
	TextOut(ps.hdc, 3, 3, asdf, strlen(asdf));

	SelectObject(ps.hdc, hOld2);
	SelectObject(ps.hdc, hOld);
	EndPaint(hwnd, &ps);
}


void TreeInsert(char *key, int data) {
	LPNODE lpnode  = (LPNODE)malloc(sizeof(NODE));
	lpnode->key    = hash((unsigned char *)key, -1);
	lpnode->data   = data;
	lpnode->lchild = NULL;
	lpnode->rchild = NULL;

	LPNODE *tmp = &root;	
	while (*tmp)
		tmp = (lpnode->key > (*tmp)->key) ? &((*tmp)->rchild) : &((*tmp)->lchild);

	*tmp = lpnode;
}


void GenerateTreeFromStringlist() {
	char asdf[32];
	int i = 0;

	FILE *file = fopen(inputfile, "r");
	if (!file)
		return;
	
	while (!feof(file)) {
		char *tmp;
		fgets(asdf, sizeof(asdf), file);
		tmp = asdf;
		while (*tmp && *tmp != 0x0A)
			tmp++;
		*tmp = 0;
		TreeInsert(asdf, i);
		i++;
	}

	numnodes = i;
	fclose(file);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
		case WM_CREATE:
			hWnd_main = hwnd;
			GenerateTreeFromStringlist();
			DSWBalanceTree(root);
			treeheight = GetHeight(root);
			GenerateCode(root);
			break;
		case WM_PAINT:
			DoPainting(hwnd);
			return 1;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
	return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	MSG msg;
	WNDCLASSEX wndclass;

	g_hInst = hInstance;

	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;	
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(0, MAKEINTRESOURCE(100));
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;
	wndclass.hIconSm       = LoadIcon(0, MAKEINTRESOURCE(100));
	RegisterClassEx(&wndclass);

	hWnd_main = CreateWindowEx(0, szAppName, szAppName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		120, 200, 550, 550, NULL, NULL, hInstance, NULL);			

	ShowWindow(hWnd_main, SW_MAXIMIZE);
	UpdateWindow(hWnd_main);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}				  

	return msg.wParam;
}

