void hsvToRgb(byte h, byte s, byte v, byte rgbColor[]){
    if(s){
        byte i = h / 43;
        byte f = (h - (i * 43)) * 6; 
        byte p = (v * (255 - s)) >> 8;
        byte q = (v * (255 - ((s * f) >> 8))) >> 8;
        byte t = (v * (255 - ((s * (255 - f)) >> 8))) >> 8;
        switch (i){
            case 0:
                rgbColor[0] = v; 
                rgbColor[1] = t; 
                rgbColor[2] = p;
                break;
            case 1:
                rgbColor[0] = q; 
                rgbColor[1] = v; 
                rgbColor[2] = p;
                break;
            case 2:
                rgbColor[0] = p; 
                rgbColor[1] = v; 
                rgbColor[2] = t;
                break;
            case 3:
                rgbColor[0] = p; 
                rgbColor[1] = q; 
                rgbColor[2] = v;
                break;
            case 4:
                rgbColor[0] = t; 
                rgbColor[1] = p; 
                rgbColor[2] = v;
                break;
            default:
                rgbColor[0] = v; 
                rgbColor[1] = p; 
                rgbColor[2] = q;
                break;
        }
    }else{
        rgbColor[0] = v;
        rgbColor[1] = v;
        rgbColor[2] = v;
    }
}

void rgbToHsv(byte r, byte g, byte b, byte hsvColor[]){
    byte min_val = std::min(std::min(r,g),b);
    byte max_val = std::max(std::max(r,g),b);
    byte diff = max_val - min_val;
    hsvColor[2] = max_val;
    if (hsvColor[2] == 0){
        hsvColor[0] = 0;
        hsvColor[1] = 0;
    }else{
        hsvColor[1] = 255 * (word)diff / hsvColor[2];
        if (hsvColor[1] == 0){
            hsvColor[0] = 0;
        }else{
            if (r == max_val)
                hsvColor[0] = 0 + ((g - b) * 43) / diff;
            else if (g == max_val)
                hsvColor[0] = 85 + ((b - r) * 43) / diff;
            else
                hsvColor[0] = 171 + ((r - g) * 43) / diff;
        }
    }
}
