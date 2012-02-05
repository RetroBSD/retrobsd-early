var Terminus12 = new Image();
Terminus12.src = 'data:image/png;base64,' +
  'iVBORw0KGgoAAAANSUhEUgAAAvoAAAAQCAYAAABnX9sYAAAABGdBTUEAALGPC/xhBQAAAAZiS0dEAAAA' +
  'AAAA+UO7fwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAAd0SU1FB9sJFxERIXansRIAAASdSURBVHja7V1b' +
  'juQgDJwe5f733FNkf6alCAX8KoNJqr52J+FlysYYh/75IQiCIAiCIAiCIAiCIAiCaHGe57myPEH5zpDd' +
  'Kjn22j3/0Hv3sDTw+Xw+6E576rwOQFte25b3PU25b7/RctxNWZ46/lbRWn6077dysJSXyrbvSc817d+1' +
  '2Ss/muO796T6R7zpPZPKj/qfPX+W8qNFJSK/79+i8tOWl/gX0S+NfbHqkMaeW8fX05Gofkblo+WnRh81' +
  'a5+HT6g1VMPX7PVBmgPJVr15ne9xQMONynK02h+EXk7ZCXrq9PZDW877Hrp+RhT2H5cUAYg8t/LP89zS' +
  'P7T+WMbXi2Zkyn/2c01Ex1u/ps5o+agt0Mjijgdo+Vv4FbV9s/ijlZlkj1qMynn0HRFRRXJypg+C6NfM' +
  'tTerraieRfpl4R9iffboZc8O/v4QrgjTm6PyRA2ePnnTZNWvqzyecmpUaQwt364yvuPi7DmY2Z6nrV04' +
  'eeeoa8d25cOVE99/jyL20v9nzVtVu4roV2X92M0eW08NkLzS6E77t2O0M7AeDXrTCzTl73ZTmvo9R5+o' +
  'FBxLGpA0oStSB67vXI20pw1p/lamHmifzzaeq43livYji4Tm2J8b9H03c5a1xspldArojs7SKJXtLhIv' +
  '9VHrGHmcXY+9z5ZdBRu6u41b3X+EXeqVl9anTByS4zdSVoQjpXX0JAWW6teOT7Pr0rxnndC78SHl7y2v' +
  'HZPW6O9qiBA5k94cPE/91hzQyGJWychLHKzKP2kTXa2PvYiVRsaoxTR6DI/Ok0Z/A1CJH+28Rr6v8/Zb' +
  'CrBJG4HdgyKV7dcMR11b9yo5WU6MpKg8uv9H5kL+1ujZG46udpM9aqHSOglZm4XI/Fiilpqo3I6pQ5FF' +
  'IOpoSeUz+YHkf0S2mREzVNRNGwiRuGAJtGiCVlIgbJU+3qV1odeLu7kYnbii+1AlYv9Un0vDH63tjp4K' +
  'Z9ulFSfPh2RIVk32zmR94wYnO+KEcFQQER+tk7AznzW3vewcsfI6RYjThp0jclH9q+7k774+Ie1Pz3Ee' +
  'fachcQTF/Ts9am1Wxia2wkagsg1BONg7n/ijb3ZMcfS5G8U6SG919lcT2hoVmu1oVG6vwliqO/tZC0KF' +
  'udam3qzicqaT743Y76bzmtS2O/1An4hmrPcrc/T57Q+BOtHM4FLqrTtvvUryyTeiVJArwkm3zlHlTQv1' +
  'p96mpvImw5MGEtWfp+tLNVlE0gvbKD1i7O0mMoM/0se/Gd/WVbOP2T9YNmoXdVpT2XZH5yZyOUsEh1UJ' +
  'RoplfS4JKPqxkneSUdGn3i0GmfmxI/lF5sdDTo2xReSYInJEPfy3thHRn7a890dvsvRP4ndv3j36k7Ew' +
  'oO2PJkIsOdczP9bMvA5uBj+jt7Jly3/G/Ho/th/NteUUTmN/vemT0glDZF2e6eRLtwYxnKfngffWxFl9' +
  'zEhbI5J3Rt4fyNpZFvxxLxpegvwhCHL++bLn3D0XB0VAEASBByM4BEHsYm9WpLbRRtLRD5Mp49aXjQn6' +
  'j3SPGSWCIAiCm1uCvCCI3Rzb3/M8qYgEQRAEQRAPwn/iUZ7PYTfH6wAAAABJRU5ErkJggg==';
Terminus12.c = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789!@#$%^&*()-=[]\\;\',./_+{}|:"<>?`~';
Terminus12.w = 8;
Terminus12.h = 16;

CanvasRenderingContext2D.prototype.drawString = function(s, f, x, y)
{
  y = Math.round(y);
  var z = x = Math.round(x), t, i, j;
  if (!f.f)
  {
    f.f = [t = 0], i = 0, j = f.c.length;
    while (++i < j)
      f.f[i] = t += f.w;
  }
  s = s.split(''), i = 0, j = s.length;
  while (i<j)
    if ((t = f.c.indexOf(s[i++])) >= 0)
      this.drawImage(f, f.f[t], 0, f.w, f.height, x, y, f.w, f.height), x += f.w;
  else if (s[i-1] == '\n')
    x = z, y += f.h;
}
