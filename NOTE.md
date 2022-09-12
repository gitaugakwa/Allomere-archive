Currently, the spectrogram is applied from the previous beat to the current beat

|------|------|------|------|
<------>......<------>

This is basically compares the content between the beats and not the beat itself
An improvement would be

|------|------|------|------|
....<----->.......<----->

Where the comparison uses n samples from the beat's position to pad the actual position
