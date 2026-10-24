#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Gör om manual/MANUAL.md (svenska) och manual/MANUAL_EN.md (engelska) till src/Faltloggen/manual_data.h.
# Kör från var som helst:  python3 tools/manual/md2c.py
#
# Varje kapitel ("## N. Titel") blir en rubrik och en text med en post per rad. Första tecknet säger vad posten är:
#   P = stycke, B = punkt, b = underpunkt (indragen), N = numrerad rad (numret står kvar i texten), G = luft.
# **fet** behålls som ** i texten (ui.h ritar fetstil). Rader i *kursiv* och "---" hoppas över.
# Tecken som typsnittet saknar byts ut: … -> ..., − -> -.
# Båda språken måste ha lika många kapitel (samma ordning); skärmen väljer MANUAL_SV eller MANUAL_EN.
import re, os, sys
rot = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

def las(fil):
    src = open(os.path.join(rot, 'manual', fil), encoding='utf-8').read()
    src = src.replace('\u2026', '...').replace('\u2212', '-')
    kap = []                      # [(titel, [poster])]
    prev_blank = True
    for raw in src.split('\n'):
        line = raw.rstrip()
        if line.startswith('## '):
            kap.append((line[3:].strip(), [])); prev_blank = True; continue
        if not kap: continue
        st = line.strip()
        if st == '---': continue
        if st.startswith('*') and st.endswith('*') and not st.startswith('**'):
            continue                                     # anteckning till programmeraren
        poster = kap[-1][1]
        if not st:
            if poster and poster[-1] != 'G': poster.append('G')
            prev_blank = True; continue
        ind = len(line) - len(line.lstrip(' '))
        m = re.match(r'^(\d+)\.\s+(.*)$', st)
        if st.startswith('- '):
            poster.append(('b' if ind >= 2 else 'B') + st[2:])
        elif m:
            poster.append('N' + m.group(1) + '. ' + m.group(2))
        elif not prev_blank and poster and poster[-1] != 'G':
            poster[-1] += ' ' + st                        # fortsättning på förra raden
        else:
            poster.append('P' + st)
        prev_blank = False
    for t, p in kap:
        while p and p[-1] == 'G': p.pop()
    return kap

def cstr(s):
    b = s.encode('utf-8')
    out = '"'
    for ch in b:
        if ch == 0x22: out += '\\"'
        elif ch == 0x5C: out += '\\\\'
        elif ch == 0x0A: out += '\\n"\n    "'
        elif 0x20 <= ch < 0x7F: out += chr(ch)
        else: out += '\\%03o' % ch
    return out + '"'

sv, en = las('MANUAL.md'), las('MANUAL_EN.md')
if len(sv) != len(en):
    sys.exit('FEL: MANUAL.md har %d kapitel men MANUAL_EN.md har %d' % (len(sv), len(en)))
h = ['// SPDX-License-Identifier: CC-BY-SA-4.0',
     '// Skapad av tools/manual/md2c.py ur manual/MANUAL.md och manual/MANUAL_EN.md – ändra inte här.',
     '// Poster: P stycke, B punkt, b underpunkt, N numrerad rad, G luft; **fet**.',
     '#pragma once', '#include "lang.h"', '', 'struct ManKap { const char *titel; const char *text; };']
for namn, kap in (('MANUAL_SV', sv), ('MANUAL_EN', en)):
    h.append('static const ManKap %s[] = {' % namn)
    for t, p in kap:
        h.append('  {%s,\n    %s},' % (cstr(t), cstr('\n'.join(p))))
    h.append('};')
h += ['static const int MAN_N = sizeof(MANUAL_SV) / sizeof(MANUAL_SV[0]);',
      'static_assert(sizeof(MANUAL_SV) == sizeof(MANUAL_EN), "manualerna har olika antal kapitel");',
      '#define MANUAL (uiLang ? MANUAL_EN : MANUAL_SV)', '']
ut = os.path.join(rot, 'src', 'Faltloggen', 'manual_data.h')
open(ut, 'w', encoding='utf-8').write('\n'.join(h))
print(len(sv), 'kapitel,', sum(len('\n'.join(p)) for _, p in sv), '+', sum(len('\n'.join(p)) for _, p in en), 'byte text ->', os.path.normpath(ut))
