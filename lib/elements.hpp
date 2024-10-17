/* _    _  _ _  _ _ ____ ____ _ ___
 * |    |  |  \/  | | __ |__/ | |  \
 * |___ |__| _/\_ | |__] |  \ | |__/
 * =================================
 * Luxigrid - Periodic Table Data
 * Copyright (c) 2024 OverScore Media - MIT License
 *
 * Data from https://en.wikipedia.org/wiki/List_of_chemical_elements
 * Scraped using scripts/generate_elements.py
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ELEMENTS_GUARD
#define ELEMENTS_GUARD

#include "Arduino.h"

struct Element {
	const String symbol;
	const int number;
	const String name;
	const String atomicWeight;
	const char group;
};

Element elements[] = {
    {"H", 1, "Hydrogen", "1.0080", 's'},
    {"He", 2, "Helium", "4.0026", 's'},
    {"Li", 3, "Lithium", "6.94", 's'},
    {"Be", 4, "Beryllium", "9.0122", 's'},
    {"B", 5, "Boron", "10.81", 'p'},
    {"C", 6, "Carbon", "12.011", 'p'},
    {"N", 7, "Nitrogen", "14.007", 'p'},
    {"O", 8, "Oxygen", "15.999", 'p'},
    {"F", 9, "Fluorine", "18.998", 'p'},
    {"Ne", 10, "Neon", "20.180", 'p'},
    {"Na", 11, "Sodium", "22.990", 's'},
    {"Mg", 12, "Magnesium", "24.305", 's'},
    {"Al", 13, "Aluminium", "26.982", 'p'},
    {"Si", 14, "Silicon", "28.085", 'p'},
    {"P", 15, "Phosphorus", "30.974", 'p'},
    {"S", 16, "Sulfur", "32.06", 'p'},
    {"Cl", 17, "Chlorine", "35.45", 'p'},
    {"Ar", 18, "Argon", "39.95", 'p'},
    {"K", 19, "Potassium", "39.098", 's'},
    {"Ca", 20, "Calcium", "40.078", 's'},
    {"Sc", 21, "Scandium", "44.956", 'd'},
    {"Ti", 22, "Titanium", "47.867", 'd'},
    {"V", 23, "Vanadium", "50.942", 'd'},
    {"Cr", 24, "Chromium", "51.996", 'd'},
    {"Mn", 25, "Manganese", "54.938", 'd'},
    {"Fe", 26, "Iron", "55.845", 'd'},
    {"Co", 27, "Cobalt", "58.933", 'd'},
    {"Ni", 28, "Nickel", "58.693", 'd'},
    {"Cu", 29, "Copper", "63.546", 'd'},
    {"Zn", 30, "Zinc", "65.38", 'd'},
    {"Ga", 31, "Gallium", "69.723", 'p'},
    {"Ge", 32, "Germanium", "72.630", 'p'},
    {"As", 33, "Arsenic", "74.922", 'p'},
    {"Se", 34, "Selenium", "78.971", 'p'},
    {"Br", 35, "Bromine", "79.904", 'p'},
    {"Kr", 36, "Krypton", "83.798", 'p'},
    {"Rb", 37, "Rubidium", "85.468", 's'},
    {"Sr", 38, "Strontium", "87.62", 's'},
    {"Y", 39, "Yttrium", "88.906", 'd'},
    {"Zr", 40, "Zirconium", "91.224", 'd'},
    {"Nb", 41, "Niobium", "92.906", 'd'},
    {"Mo", 42, "Molybdenum", "95.95", 'd'},
    {"Tc", 43, "Technetium", "97", 'd'},
    {"Ru", 44, "Ruthenium", "101.07", 'd'},
    {"Rh", 45, "Rhodium", "102.91", 'd'},
    {"Pd", 46, "Palladium", "106.42", 'd'},
    {"Ag", 47, "Silver", "107.87", 'd'},
    {"Cd", 48, "Cadmium", "112.41", 'd'},
    {"In", 49, "Indium", "114.82", 'p'},
    {"Sn", 50, "Tin", "118.71", 'p'},
    {"Sb", 51, "Antimony", "121.76", 'p'},
    {"Te", 52, "Tellurium", "127.60", 'p'},
    {"I", 53, "Iodine", "126.90", 'p'},
    {"Xe", 54, "Xenon", "131.29", 'p'},
    {"Cs", 55, "Caesium", "132.91", 's'},
    {"Ba", 56, "Barium", "137.33", 's'},
    {"La", 57, "Lanthanum", "138.91", 'f'},
    {"Ce", 58, "Cerium", "140.12", 'f'},
    {"Pr", 59, "Praseodymium", "140.91", 'f'},
    {"Nd", 60, "Neodymium", "144.24", 'f'},
    {"Pm", 61, "Promethium", "145", 'f'},
    {"Sm", 62, "Samarium", "150.36", 'f'},
    {"Eu", 63, "Europium", "151.96", 'f'},
    {"Gd", 64, "Gadolinium", "157.25", 'f'},
    {"Tb", 65, "Terbium", "158.93", 'f'},
    {"Dy", 66, "Dysprosium", "162.50", 'f'},
    {"Ho", 67, "Holmium", "164.93", 'f'},
    {"Er", 68, "Erbium", "167.26", 'f'},
    {"Tm", 69, "Thulium", "168.93", 'f'},
    {"Yb", 70, "Ytterbium", "173.05", 'f'},
    {"Lu", 71, "Lutetium", "174.97", 'd'},
    {"Hf", 72, "Hafnium", "178.49", 'd'},
    {"Ta", 73, "Tantalum", "180.95", 'd'},
    {"W", 74, "Tungsten", "183.84", 'd'},
    {"Re", 75, "Rhenium", "186.21", 'd'},
    {"Os", 76, "Osmium", "190.23", 'd'},
    {"Ir", 77, "Iridium", "192.22", 'd'},
    {"Pt", 78, "Platinum", "195.08", 'd'},
    {"Au", 79, "Gold", "196.97", 'd'},
    {"Hg", 80, "Mercury", "200.59", 'd'},
    {"Tl", 81, "Thallium", "204.38", 'p'},
    {"Pb", 82, "Lead", "207.2", 'p'},
    {"Bi", 83, "Bismuth", "208.98", 'p'},
    {"Po", 84, "Polonium", "209", 'p'},
    {"At", 85, "Astatine", "210", 'p'},
    {"Rn", 86, "Radon", "222", 'p'},
    {"Fr", 87, "Francium", "223", 's'},
    {"Ra", 88, "Radium", "226", 's'},
    {"Ac", 89, "Actinium", "227", 'f'},
    {"Th", 90, "Thorium", "232.04", 'f'},
    {"Pa", 91, "Protactinium", "231.04", 'f'},
    {"U", 92, "Uranium", "238.03", 'f'},
    {"Np", 93, "Neptunium", "237", 'f'},
    {"Pu", 94, "Plutonium", "244", 'f'},
    {"Am", 95, "Americium", "243", 'f'},
    {"Cm", 96, "Curium", "247", 'f'},
    {"Bk", 97, "Berkelium", "247", 'f'},
    {"Cf", 98, "Californium", "251", 'f'},
    {"Es", 99, "Einsteinium", "252", 'f'},
    {"Fm", 100, "Fermium", "257", 'f'},
    {"Md", 101, "Mendelevium", "258", 'f'},
    {"No", 102, "Nobelium", "259", 'f'},
    {"Lr", 103, "Lawrencium", "266", 'd'},
    {"Rf", 104, "Rutherfordium", "267", 'd'},
    {"Db", 105, "Dubnium", "268", 'd'},
    {"Sg", 106, "Seaborgium", "269", 'd'},
    {"Bh", 107, "Bohrium", "270", 'd'},
    {"Hs", 108, "Hassium", "271", 'd'},
    {"Mt", 109, "Meitnerium", "278", 'd'},
    {"Ds", 110, "Darmstadtium", "281", 'd'},
    {"Rg", 111, "Roentgenium", "282", 'd'},
    {"Cn", 112, "Copernicium", "285", 'd'},
    {"Nh", 113, "Nihonium", "286", 'p'},
    {"Fl", 114, "Flerovium", "289", 'p'},
    {"Mc", 115, "Moscovium", "290", 'p'},
    {"Lv", 116, "Livermorium", "293", 'p'},
    {"Ts", 117, "Tennessine", "294", 'p'},
    {"Og", 118, "Oganesson", "294", 'p'}};

#endif