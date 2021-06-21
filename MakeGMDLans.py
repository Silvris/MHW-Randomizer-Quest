import shutil

nonEngLans = [
    "ara",
    "chS",
    "chT",
    "fre",
    "ger",
    "ita",
    "jpn",
    "kor",
    "pol",
    "ptB",
    "rus",
    "spa"
]

gmds = [
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99994_eng.gmd",
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99995_eng.gmd",
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99996_eng.gmd",
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99997_eng.gmd",
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99998_eng.gmd",
    r"C:\Users\Owner\Documents\GitHub\MHW-Randomizer-Quest\nativePC\common\text\quest\q99999_eng.gmd"
]

for gmd in gmds:
    for lan in nonEngLans:
        shutil.copyfile(gmd,gmd.replace("eng",lan))