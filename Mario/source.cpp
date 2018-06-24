#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<SDL.h>

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define SZYBKOSC_MARIO 4 //odwrotnie proporcjonalne
#define SZYBKOSC_ZMIAN_ANIMACJI 50//odwrotnie proporcjonalne
#define PULAP_MARIO 500
#define WYSOKOSC_MARIO 16
#define WYSOKOSC_KLOCKA 16
#define DLUGOSC_KLOCKA 16
#define DLUGOSC_DZIURY 48
#define DLUGOSC_MARIO 16
#define GLEBA SCREEN_HEIGHT - 63
#define WYSOKOSC_MURKA 60
#define SZYBKOSC_STWORKA 7 //odwrotnie proporcjonalnie
#define FPS 60

struct point { int x; int y; };
struct rama { int xp; int xk; };

struct mario {
	rama ramkaekranu; point pozycja; bool trybwprawo; bool trybwlewo; bool trybskakania; bool trybstanialewo; bool trybstaniaprawo;
	int pomocnicza; int stabilizacjaanimacji; int pulap; int grawitacyjna; double czas; int liczbazyc; point pozycjapoczatkowa; int dlugoscmapy; double poczatkowyczas;
	int ktoryetap; int kolizja;
};

struct animacje {
	SDL_Surface * marioidzieprawo; SDL_Surface * marioidzieprawo2; SDL_Surface * marioidzielewo; SDL_Surface * marioidzielewo2;
	SDL_Surface * mariostoiprawo; SDL_Surface * mariostoilewo; SDL_Surface * marioskaczeprawo; SDL_Surface * marioskaczelewo;
	SDL_Surface * klocek; SDL_Surface * dol; SDL_Surface * murek; SDL_Surface * mariotracizycie; SDL_Surface *charset;
	SDL_Surface *stworekwlewo; SDL_Surface *stworekwprawo;
};

struct stwor { int poczatek; int koniec; int wktora; int pozycja; };


struct obiekty { int *dziury; point *klockiwiszace; int iloscdziur; int iloscklockowwiszacych; int iloscstworkow; stwor *stworki; int czas; int pomocnicza; };

// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
	SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
};


// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) 
// b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostok¹ta o d³ugoœci boków l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};


int maximum(int a, int b)
{
	if (a > b) return a; return b;
}
int minimum(int a, int b)
{
	if (a > b) return b; return a;
}

void grawitacja(mario *Mario, int czas)
{
	Mario->grawitacyjna += czas;
	Mario->pozycja.y += Mario->grawitacyjna / SZYBKOSC_MARIO;
	Mario->grawitacyjna %= SZYBKOSC_MARIO;
	Mario->trybskakania = 0;

}

int wart_bezw(int x)
{
	if (x > 0)
		return x;
	return -x;
}



bool szukaj(int l, int p, obiekty *Obiekty, point punkt)
{
	if (l > p)
		return 1;

	int srodek = (l + p) / 2;

	if (wart_bezw(Obiekty->klockiwiszace[srodek].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO && wart_bezw(Obiekty->klockiwiszace[srodek].y - punkt.y) < WYSOKOSC_KLOCKA + WYSOKOSC_MARIO)
		return 0;


	if (l == p)
		return 1;

	if (wart_bezw(Obiekty->klockiwiszace[l].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO&&wart_bezw(Obiekty->klockiwiszace[p].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO)
	{
		for (int k = l; k <= p; k++)
			if (wart_bezw(Obiekty->klockiwiszace[k].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO&&wart_bezw(Obiekty->klockiwiszace[k].y - punkt.y) < WYSOKOSC_KLOCKA + WYSOKOSC_MARIO)
				return 0;
		for (int k = l - 1; k >= 0; k--)
			if (Obiekty->klockiwiszace[k].x == Obiekty->klockiwiszace[l].x)
			{
				if (wart_bezw(Obiekty->klockiwiszace[k].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO&&wart_bezw(Obiekty->klockiwiszace[k].y - punkt.y) < WYSOKOSC_KLOCKA + WYSOKOSC_MARIO)
					return 0;

			}
			else
				break;

		for (int k = p + 1; k <= Obiekty->iloscklockowwiszacych; k++)
			if (Obiekty->klockiwiszace[k].x == Obiekty->klockiwiszace[p].x)
			{
				if (wart_bezw(Obiekty->klockiwiszace[k].x - punkt.x) < DLUGOSC_KLOCKA + DLUGOSC_MARIO&&wart_bezw(Obiekty->klockiwiszace[k].y - punkt.y) < WYSOKOSC_KLOCKA + WYSOKOSC_MARIO)
					return 0;

			}
			else
				break;


		return 1;
	}



	if (Obiekty->klockiwiszace[srodek].x + DLUGOSC_KLOCKA < punkt.x - DLUGOSC_MARIO)
		return szukaj(srodek + 1, p, Obiekty, punkt);
	else
		return szukaj(l, srodek - 1, Obiekty, punkt);

}




bool szukajdziur(int l, int p, obiekty *Obiekty, int pozycja)
{
	int srodek = (l + p) / 2;
	if (l > p)
		return 0;

	if (l == p)
	{
		if (wart_bezw(pozycja - Obiekty->dziury[l]) < DLUGOSC_DZIURY)
			return 1;
		else
			return 0;
	}
	if (wart_bezw(pozycja - Obiekty->dziury[srodek]) < DLUGOSC_DZIURY)
		return 1;
	if (pozycja > Obiekty->dziury[srodek])
		return szukajdziur(srodek + 1, p, Obiekty, pozycja);
	else
		return szukajdziur(l, srodek - 1, Obiekty, pozycja);

}


long long odleglosc(point a, point b)
{
	if (wart_bezw(a.x - b.x) > 1000 || wart_bezw((a.y - b.y)))
		return 1000;
	return (sqrtl((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y)));

}


bool kolizja_z_potworem(int l, int p, stwor *stworki, point punkt)
{
	if (l > p)
		return 0;
	point pom;
	int srodek = (l + p) / 2;
	pom.x = stworki[srodek].pozycja; pom.y = GLEBA - WYSOKOSC_MARIO;

	if (odleglosc(punkt, pom) < 2 * DLUGOSC_MARIO)
		return 1;
	if (l == p)
		return 0;
	if (punkt.x < stworki[srodek].pozycja)
		return kolizja_z_potworem(l, srodek - 1, stworki, punkt);
	else
		return kolizja_z_potworem(srodek + 1, p, stworki, punkt);


}



void MENU(SDL_Surface *screen, animacje *animacjemario)
{


	char text[64];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	SDL_FillRect(screen, NULL, czarny);
	sprintf(text, "n - NOWA GRA");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 8, text, animacjemario->charset);
	sprintf(text, "ESC - WYJDZ Z GRY");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 8, text, animacjemario->charset);



}

char *zamien(int ktoryetap)
{
	char liczba[10];
	int j = -1;
	while (ktoryetap > 0)
	{
		j++;
		liczba[j] = ktoryetap % 10 + '0';
		ktoryetap /= 10;
	}

	j++;

	liczba[j] = 0;
	char *napis = (char *)malloc(sizeof(char)*(4 + strlen(liczba) + 5));

	napis[0] = 'e'; napis[1] = 't'; napis[2] = 'a'; napis[3] = 'p';
	for (int j = 0, k = strlen(liczba) - 1; k >= 0; k--, j++)
		napis[4 + j] = liczba[k];
	napis[4 + strlen(liczba)] = '.'; napis[5 + strlen(liczba)] = 't'; napis[6 + strlen(liczba)] = 'x'; napis[7 + strlen(liczba)] = 't';
	napis[8 + strlen(liczba)] = 0;

	return napis;

}





void wczytaj_dane(mario *Mario, obiekty *Obiekty, int ktoryetap, bool *czywyswietlacmenu)
{
	char *napis = zamien(ktoryetap);
	FILE *plik = fopen(napis, "r");

	if (plik != NULL)
	{
		if (Obiekty->dziury)free(Obiekty->dziury);
		if (Obiekty->klockiwiszace)free(Obiekty->klockiwiszace);
		if (Obiekty->stworki)free(Obiekty->stworki);
		fscanf(plik, "%lf%i%i%i", &Mario->poczatkowyczas, &Mario->dlugoscmapy, &Mario->liczbazyc, &Mario->pozycjapoczatkowa);
		fscanf(plik, "%i", &Obiekty->iloscklockowwiszacych);
		Obiekty->klockiwiszace = (point *)malloc(sizeof(point)*Obiekty->iloscklockowwiszacych);
		for (int k = 0; k < Obiekty->iloscklockowwiszacych; k++)
			fscanf(plik, "%i%i", &Obiekty->klockiwiszace[k].x, &Obiekty->klockiwiszace[k].y);

		fscanf(plik, "%i", &Obiekty->iloscdziur);
		Obiekty->dziury = (int *)malloc(sizeof(int)*Obiekty->iloscdziur);
		for (int k = 0; k < Obiekty->iloscdziur; k++)
			fscanf(plik, "%i", &Obiekty->dziury[k]);

		fscanf(plik, "%i", &Obiekty->iloscstworkow);
		Obiekty->stworki = (stwor *)malloc(sizeof(stwor)*Obiekty->iloscstworkow);
		for (int k = 0; k < Obiekty->iloscstworkow; k++)
		{
			fscanf(plik, "%i%i", &Obiekty->stworki[k].poczatek, &Obiekty->stworki[k].koniec);
			Obiekty->stworki[k].pozycja = Obiekty->stworki[k].poczatek;
			Obiekty->stworki[k].wktora = 1;
		}
		Mario->pozycja.x = Mario->pozycjapoczatkowa.x;
		Mario->pozycja.y = GLEBA - WYSOKOSC_MARIO;
		Mario->trybskakania = 0; Mario->trybstaniaprawo = 1; Mario->trybstanialewo = 0; Mario->trybwlewo = 0; Mario->trybwprawo = 0;
		Mario->pozycja.y = GLEBA - WYSOKOSC_MARIO; Mario->pomocnicza = 0; Mario->stabilizacjaanimacji = 0; Mario->pulap = 0; Mario->grawitacyjna = 0;
		Mario->ramkaekranu.xp = 0; Mario->ramkaekranu.xk = SCREEN_WIDTH; Mario->czas = Mario->poczatkowyczas; Mario->pomocnicza = 0;
		Mario->pozycjapoczatkowa.y = GLEBA - WYSOKOSC_MARIO - 1; Obiekty->czas = 0; Obiekty->pomocnicza = 0;




		fclose(plik);
	}
	else
		*czywyswietlacmenu = 1;

}


void wyzeruj_mario(mario *Mario)
{
	Mario->pozycja.x = Mario->pozycjapoczatkowa.x; Mario->pozycja.y = Mario->pozycjapoczatkowa.y; Mario->liczbazyc -= 1; Mario->ramkaekranu.xp = Mario->pozycjapoczatkowa.x - 10;
	Mario->ramkaekranu.xk = Mario->ramkaekranu.xp + SCREEN_WIDTH; Mario->trybstaniaprawo = 1; Mario->trybstanialewo = 0; Mario->czas = Mario->poczatkowyczas; Mario->kolizja = 0;

}

bool czy_mozna_w_prawo(obiekty *Obiekty, mario *Mario, int oile)
{
	point pom;
	pom.x = Mario->pozycja.x + oile; pom.y = Mario->pozycja.y;
	if (Mario->pozycja.y  > GLEBA - WYSOKOSC_MARIO)
		return 0;
	return szukaj(0, Obiekty->iloscklockowwiszacych - 1, Obiekty, pom);

}

bool czy_mozna_w_lewo(obiekty *Obiekty, mario *Mario, int oile)
{

	point pom;
	if (Mario->pozycja.y  > GLEBA - WYSOKOSC_MARIO)
		return 0;
	pom.x = Mario->pozycja.x - oile; pom.y = Mario->pozycja.y;
	return szukaj(0, Obiekty->iloscklockowwiszacych - 1, Obiekty, pom);

}

bool czy_mozna_w_gore(obiekty *Obiekty, mario *Mario, int oile)
{
	if (Mario->pozycja.y  > GLEBA - WYSOKOSC_MARIO)
		return 0;
	point pom;
	pom.x = Mario->pozycja.x; pom.y = Mario->pozycja.y - oile;
	return szukaj(0, Obiekty->iloscklockowwiszacych - 1, Obiekty, pom);

}


bool czy_mozna_w_dol(obiekty *Obiekty, mario *Mario, int oile)
{

	point pom;
	pom.x = Mario->pozycja.x; pom.y = Mario->pozycja.y + oile;
	if (Mario->pozycja.y >= GLEBA - WYSOKOSC_MARIO)
	{
		if (szukajdziur(0, Obiekty->iloscdziur - 1, Obiekty, Mario->pozycja.x) == 1)
			return 1;
		else
		{
			Mario->pozycja.y = GLEBA - WYSOKOSC_MARIO;
			return 0;
		}

	}
	return szukaj(0, Obiekty->iloscklockowwiszacych - 1, Obiekty, pom);

}

void sterujMario(SDL_Surface *screen, animacje *animacjemario, mario *Mario, int t1, int t2, obiekty *Obiekty, int *wyjdz, bool *czywyswietlacmenu)
{

	if (kolizja_z_potworem(0, Obiekty->iloscstworkow - 1, Obiekty->stworki, Mario->pozycja))
		Mario->kolizja = 1;




	if (Mario->pozycja.y > GLEBA + WYSOKOSC_MURKA)
	{
		if (GLEBA + (GLEBA + WYSOKOSC_MURKA - Mario->pozycja.y) > 0)
			DrawSurface(screen, animacjemario->mariotracizycie, Mario->pozycja.x - Mario->ramkaekranu.xp, GLEBA + (GLEBA + WYSOKOSC_MURKA - Mario->pozycja.y));
		else
		{
			wyzeruj_mario(Mario);
			if (Mario->liczbazyc == 0)
				*czywyswietlacmenu = true;
		}
	}

	if (Mario->czas < 0 || Mario->kolizja == 1)
	{
		Mario->pomocnicza += t2 - t1;
		Mario->pozycja.y -= Mario->pomocnicza / SZYBKOSC_MARIO;
		DrawSurface(screen, animacjemario->mariotracizycie, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
		Mario->czas = 0;
		Mario->pomocnicza %= SZYBKOSC_MARIO;
		if (Mario->pozycja.y < 0)
			wyzeruj_mario(Mario);
		if (Mario->liczbazyc == 0)
			*czywyswietlacmenu = true;
		return;
	}


	if (Mario->pozycja.x >= Mario->dlugoscmapy)
	{
		Mario->ktoryetap += 1;
		wczytaj_dane(Mario, Obiekty, Mario->ktoryetap, czywyswietlacmenu);

	}

	bool sr, sl, su, sd;
	sr = czy_mozna_w_prawo(Obiekty, Mario, (Mario->pomocnicza + t2 - t1) / SZYBKOSC_MARIO);
	sl = czy_mozna_w_lewo(Obiekty, Mario, (Mario->pomocnicza + t2 - t1) / SZYBKOSC_MARIO);
	su = czy_mozna_w_gore(Obiekty, Mario, (Mario->pomocnicza + t2 - t1) / SZYBKOSC_MARIO);
	if (su == 0)
		Mario->trybskakania = 0;
	sd = czy_mozna_w_dol(Obiekty, Mario, (Mario->grawitacyjna + t2 - t1) / SZYBKOSC_MARIO);
	if (sd == 0)
		Mario->pulap = 0;


	if ((Mario->trybskakania == 1 || sd == 1))
	{
		Mario->pomocnicza += (t2 - t1);

		if (sd&&Mario->trybskakania == 0)
			grawitacja(Mario, t2 - t1);
		else
			if (Mario->trybskakania == 1 && su)
			{
				if (Mario->pulap > PULAP_MARIO)
					Mario->trybskakania = 0;
				else
				{
					Mario->pozycja.y -= Mario->pomocnicza / SZYBKOSC_MARIO;
					Mario->pulap += Mario->pomocnicza;
				}
			}
		if (Mario->trybwprawo&&sr == 1)
		{
			Mario->pozycja.x += Mario->pomocnicza / SZYBKOSC_MARIO;
			if (Mario->pozycja.x > (Mario->ramkaekranu.xk + Mario->ramkaekranu.xp) / 2)
			{
				Mario->ramkaekranu.xk = Mario->pozycja.x + SCREEN_WIDTH / 2;
				Mario->ramkaekranu.xp = Mario->ramkaekranu.xk - SCREEN_WIDTH;

			}
		}
		if (Mario->trybwlewo&&sl == 1)
			Mario->pozycja.x = maximum(Mario->ramkaekranu.xp, Mario->pozycja.x - Mario->pomocnicza / SZYBKOSC_MARIO);
		if (Mario->trybstaniaprawo == 1 || Mario->trybwprawo == 1)
		{
			DrawSurface(screen, animacjemario->marioskaczeprawo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			Mario->trybstaniaprawo = 1; Mario->trybstanialewo = 0;
		}
		else
		{
			DrawSurface(screen, animacjemario->marioskaczelewo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			Mario->trybstaniaprawo = 0; Mario->trybstanialewo = 1;
		}
		Mario->pomocnicza %= SZYBKOSC_MARIO;
	}
	else
	{


		if (Mario->trybstaniaprawo)
			DrawSurface(screen, animacjemario->mariostoiprawo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);

		if (Mario->trybstanialewo)
			DrawSurface(screen, animacjemario->mariostoilewo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
		if (Mario->trybwprawo)
		{
			Mario->pomocnicza += (t2 - t1);
			Mario->stabilizacjaanimacji += Mario->pomocnicza;
			if (sr)
				Mario->pozycja.x += Mario->pomocnicza / SZYBKOSC_MARIO;
			if (Mario->pozycja.x > (Mario->ramkaekranu.xk + Mario->ramkaekranu.xp) / 2)
			{
				Mario->ramkaekranu.xk = Mario->pozycja.x + SCREEN_WIDTH / 2;
				Mario->ramkaekranu.xp = Mario->ramkaekranu.xk - SCREEN_WIDTH;

			}
			if ((Mario->stabilizacjaanimacji / (SZYBKOSC_ZMIAN_ANIMACJI * SZYBKOSC_MARIO)) % 2 == 1)
				DrawSurface(screen, animacjemario->marioidzieprawo2, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			else
				DrawSurface(screen, animacjemario->marioidzieprawo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			Mario->pomocnicza %= SZYBKOSC_MARIO; Mario->trybstaniaprawo = 1; Mario->trybstanialewo = 0;
		}

		if (Mario->trybwlewo)
		{
			Mario->pomocnicza += (t2 - t1);
			Mario->stabilizacjaanimacji += Mario->pomocnicza;
			if (sl)
				Mario->pozycja.x = maximum(Mario->ramkaekranu.xp, Mario->pozycja.x - Mario->pomocnicza / SZYBKOSC_MARIO);
			if ((Mario->stabilizacjaanimacji / (SZYBKOSC_ZMIAN_ANIMACJI * SZYBKOSC_MARIO)) % 2 == 1)
				DrawSurface(screen, animacjemario->marioidzielewo2, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			else
				DrawSurface(screen, animacjemario->marioidzielewo, Mario->pozycja.x - Mario->ramkaekranu.xp, Mario->pozycja.y);
			Mario->pomocnicza %= SZYBKOSC_MARIO; Mario->trybstanialewo = 1; Mario->trybstaniaprawo = 0;
		}
	}

}


void zwolinj_powierzchnie(animacje *Animacje, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Window *window, SDL_Renderer *renderer) {
	if (Animacje->charset)SDL_FreeSurface(Animacje->charset);
	if (screen)SDL_FreeSurface(screen);
	if (Animacje->dol)SDL_FreeSurface(Animacje->dol);
	if (Animacje->klocek)SDL_FreeSurface(Animacje->klocek);
	if (Animacje->marioidzielewo)SDL_FreeSurface(Animacje->marioidzielewo);
	if (Animacje->marioidzielewo2)SDL_FreeSurface(Animacje->marioidzielewo2);
	if (Animacje->marioidzieprawo)SDL_FreeSurface(Animacje->marioidzieprawo);
	if (Animacje->marioidzieprawo2)SDL_FreeSurface(Animacje->marioidzieprawo2);
	if (Animacje->marioskaczelewo)SDL_FreeSurface(Animacje->marioskaczelewo);
	if (Animacje->marioskaczeprawo)SDL_FreeSurface(Animacje->marioskaczeprawo);
	if (Animacje->mariostoilewo)SDL_FreeSurface(Animacje->mariostoilewo);
	if (Animacje->mariostoiprawo)SDL_FreeSurface(Animacje->mariostoiprawo);
	if (Animacje->murek)SDL_FreeSurface(Animacje->murek);
	if (Animacje->mariotracizycie)SDL_FreeSurface(Animacje->mariotracizycie);
	if (Animacje->stworekwlewo)SDL_FreeSurface(Animacje->stworekwlewo);
	if (Animacje->stworekwprawo)SDL_FreeSurface(Animacje->stworekwprawo);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void zapisz_stan(mario *Mario, mario *Zapis)
{
	Zapis->czas = Mario->czas;
	Zapis->grawitacyjna = Mario->grawitacyjna;
	Zapis->ktoryetap = Mario->ktoryetap;
	Zapis->liczbazyc = Mario->liczbazyc;
	Zapis->pomocnicza = Mario->pomocnicza;
	Zapis->pozycja.x = Mario->pozycja.x;
	Zapis->pozycja.y = Mario->pozycja.y;
	Zapis->pulap = Mario->pulap;
	Zapis->ramkaekranu.xk = Mario->ramkaekranu.xk;
	Zapis->ramkaekranu.xp = Mario->ramkaekranu.xp;
	Zapis->stabilizacjaanimacji = Mario->stabilizacjaanimacji;
	Zapis->trybskakania = Mario->trybskakania;
	Zapis->trybstanialewo = Mario->trybstanialewo;
	Zapis->trybstaniaprawo = Mario->trybstaniaprawo;
	Zapis->trybwlewo = Mario->trybwlewo;
	Zapis->trybwprawo = Mario->trybwprawo;
}

void wczytaj_stan(mario *Mario, mario *Zapis)
{
	Mario->czas = Zapis->czas;
	Mario->grawitacyjna = Zapis->grawitacyjna;
	Mario->ktoryetap = Zapis->ktoryetap;
	Mario->liczbazyc = Zapis->liczbazyc;
	Mario->pomocnicza = Zapis->pomocnicza;
	Mario->pozycja.x = Zapis->pozycja.x;
	Mario->pozycja.y = Zapis->pozycja.y;
	Mario->pulap = Zapis->pulap;
	Mario->ramkaekranu.xk = Zapis->ramkaekranu.xk;
	Mario->ramkaekranu.xp = Zapis->ramkaekranu.xp;
	Mario->stabilizacjaanimacji = Zapis->stabilizacjaanimacji;
	Mario->trybskakania = Zapis->trybskakania;
	Mario->trybstanialewo = Zapis->trybstanialewo;
	Mario->trybstaniaprawo = Zapis->trybstaniaprawo;
	Mario->trybwlewo = Zapis->trybwlewo;
	Mario->trybwprawo = Zapis->trybwprawo;
}


void wczytaj_animacje(animacje *animacjemario, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Window *window, SDL_Renderer *renderer)
{
	animacjemario->mariotracizycie = SDL_LoadBMP("./mariotracizycie.bmp");
	if (animacjemario->mariotracizycie == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->dol = SDL_LoadBMP("./dol.bmp");
	if (animacjemario->dol == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->murek = SDL_LoadBMP("./murek.bmp");
	if (animacjemario->murek == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->klocek = SDL_LoadBMP("./klocek.bmp");
	if (animacjemario->klocek == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->marioidzielewo = SDL_LoadBMP("./marioidzielewo.bmp");
	if (animacjemario->marioidzielewo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->marioidzieprawo = SDL_LoadBMP("./marioidzieprawo.bmp");
	if (animacjemario->marioidzieprawo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);


	animacjemario->marioskaczelewo = SDL_LoadBMP("./marioskaczelewo.bmp");
	if (animacjemario->marioskaczelewo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->marioskaczeprawo = SDL_LoadBMP("./marioskaczeprawo.bmp");
	if (animacjemario->marioskaczeprawo == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->mariostoilewo = SDL_LoadBMP("./mariostoilewo.bmp");
	if (animacjemario->mariostoilewo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->mariostoiprawo = SDL_LoadBMP("./mariostoiprawo.bmp");
	if (animacjemario->mariostoiprawo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->marioidzielewo2 = SDL_LoadBMP("./marioidzielewo2.bmp");
	if (animacjemario->marioidzielewo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->marioidzieprawo2 = SDL_LoadBMP("./marioidzieprawo2.bmp");
	if (animacjemario->marioidzieprawo == NULL) zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->stworekwlewo = SDL_LoadBMP("./stworekwlewo.bmp");
	if (animacjemario->stworekwlewo == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);

	animacjemario->stworekwprawo = SDL_LoadBMP("./stworekwprawo.bmp");
	if (animacjemario->stworekwprawo == NULL)zwolinj_powierzchnie(animacjemario, screen, scrtex, window, renderer);




}


void zmien_pozycje_potworow(stwor *stworki, int iloscstworkow, int *time, int oile, int *pomocnicza)
{
	*time += oile;
	*pomocnicza += oile;

	for (int k = 0; k < iloscstworkow; k++)
	{
		stworki[k].pozycja += stworki[k].wktora*(*pomocnicza) / SZYBKOSC_STWORKA;

		if (stworki[k].pozycja >= stworki[k].koniec  && stworki[k].wktora == 1)
			stworki[k].wktora = -1;
		else
		{
			if (stworki[k].pozycja <= stworki[k].poczatek  && stworki[k].wktora == (-1))
				stworki[k].wktora = 1;
		}
	}

	(*pomocnicza) %= SZYBKOSC_STWORKA;
}


void narysuj_potworki(stwor *stworki, int iloscstworkow, int time, SDL_Surface *screen, animacje *animacjemario, int ramka)
{

	for (int k = 0; k < iloscstworkow; k++)
	{
		if (time / (SZYBKOSC_STWORKA*SZYBKOSC_ZMIAN_ANIMACJI) % 2 == 1)
			DrawSurface(screen, animacjemario->stworekwprawo, stworki[k].pozycja - ramka, GLEBA - WYSOKOSC_MARIO);
		else
			DrawSurface(screen, animacjemario->stworekwlewo, stworki[k].pozycja - ramka, GLEBA - WYSOKOSC_MARIO);

	}
}





// main sds
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	mario Mario, Zapis; obiekty Obiekty;
	Zapis.ktoryetap = 0;

	Mario.ktoryetap = 1; Obiekty.dziury = NULL; Obiekty.klockiwiszace = NULL; Obiekty.stworki = NULL;

	bool czywyswietlacmenu = 1;
	animacje animacjemario;
	wczytaj_dane(&Mario, &Obiekty, 2, &czywyswietlacmenu);

	int t1, t2, quit, rc;
	double delta, worldTime, distance;
	SDL_Event event;
	SDL_Surface *screen;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// tryb pe³noekranowy
	//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
	//	                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Szablon do zdania drugiego 2014");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	// wczytanie obrazka cs8x8.bmp
	animacjemario.charset = SDL_LoadBMP("./cs8x8.bmp");
	if (animacjemario.charset == NULL)zwolinj_powierzchnie(&animacjemario, screen, scrtex, window, renderer);

	// wy³¹czenie widocznoœci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);
	SDL_SetColorKey(animacjemario.charset, true, 0x000000);




	wczytaj_animacje(&animacjemario, screen, scrtex, window, renderer);





	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	t1 = SDL_GetTicks();

	quit = 0;
	worldTime = 0;
	distance = 0;

	while (!quit) {
		t2 = SDL_GetTicks();


		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplyna³ od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach

		//akcja

		


		delta = (t2 - t1) * 0.001;
		if (delta >= 1.00 / FPS)
		{
			SDL_FillRect(screen, NULL, niebieski);
			if (czywyswietlacmenu == 0)
				Mario.czas -= delta;


			for (int k = 0; k < Obiekty.iloscklockowwiszacych; k++)
			{
				DrawSurface(screen, animacjemario.klocek, Obiekty.klockiwiszace[k].x - Mario.ramkaekranu.xp, Obiekty.klockiwiszace[k].y);
			}
			DrawSurface(screen, animacjemario.murek, (SCREEN_WIDTH / 2) - ((Mario.ramkaekranu.xp) % 32), SCREEN_HEIGHT - 29);
			for (int k = 0; k < Obiekty.iloscdziur; k++)
			{
				DrawSurface(screen, animacjemario.dol, Obiekty.dziury[k] - Mario.ramkaekranu.xp, GLEBA + 33);
			}


			zmien_pozycje_potworow(Obiekty.stworki, Obiekty.iloscstworkow, &Obiekty.czas, t2 - t1, &Obiekty.pomocnicza);
			narysuj_potworki(Obiekty.stworki, Obiekty.iloscstworkow, Obiekty.czas, screen, &animacjemario, Mario.ramkaekranu.xp);



			if (czywyswietlacmenu == 0)
			{
				sterujMario(screen, &animacjemario, &Mario, t1, t2, &Obiekty, &quit, &czywyswietlacmenu);
				DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czarny, czarny);
				sprintf(text, "Pozostalo %.1lf s  Do mety pozostalo %i m", Mario.czas, Mario.dlugoscmapy - Mario.pozycja.x);
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, animacjemario.charset);
				sprintf(text, "Liczba zyc: %i  etap:%i", Mario.liczbazyc, Mario.ktoryetap);
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, animacjemario.charset);

			}
			else
			{
				MENU(screen, &animacjemario);
			}
			t1 = t2;


			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			//		SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			// obs³uga zdarzeñ (o ile jakieœ zasz³y)
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_KEYUP:
					if (event.key.keysym.sym == SDLK_RIGHT) { Mario.trybwprawo = 0; Mario.pomocnicza = 0; Mario.grawitacyjna = 0; }
					if (event.key.keysym.sym == SDLK_LEFT) { Mario.trybwlewo = 0; Mario.pomocnicza = 0; Mario.grawitacyjna = 0; }
					if (event.key.keysym.sym == SDLK_UP) { Mario.trybskakania = 0; Mario.pomocnicza = 0; Mario.grawitacyjna = 0; }
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					if (event.key.keysym.sym == SDLK_n) { Mario.ktoryetap = 1; czywyswietlacmenu = 0; wczytaj_dane(&Mario, &Obiekty, 1, &czywyswietlacmenu); Mario.czas = Mario.poczatkowyczas; Zapis.ktoryetap = 0; }
					if (event.key.keysym.sym == SDLK_RIGHT) { Mario.trybwprawo = 1; Mario.trybstaniaprawo = 0; Mario.trybstanialewo = 0; }
					if (event.key.keysym.sym == SDLK_LEFT) { Mario.trybwlewo = 1; Mario.trybstaniaprawo = 0; Mario.trybstanialewo = 0; }
					if (event.key.keysym.sym == SDLK_UP&&Mario.pulap == 0) { Mario.trybskakania = 1; }
					if (event.key.keysym.sym == SDLK_s) { zapisz_stan(&Mario, &Zapis); }
					if (event.key.keysym.sym == SDLK_l) {
						if (Zapis.ktoryetap == Mario.ktoryetap)wczytaj_stan(&Mario, &Zapis);
					}
					break;

				case SDL_QUIT:
					quit = 1;
					break;
				};
			};
		}
	};
	zwolinj_powierzchnie(&animacjemario, screen, scrtex, window, renderer);
	if (Obiekty.dziury)free(Obiekty.dziury);
	if (Obiekty.klockiwiszace)free(Obiekty.klockiwiszace);
	if (Obiekty.stworki)free(Obiekty.stworki);

	return 0;
};
