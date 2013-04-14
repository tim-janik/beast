  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.37540151883546802e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.62459848116453198e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     47684.808807454952,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.02108253128409412e-02, +2.89774652464423688e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.56754761905262929e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     47684.808807454952,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -9.24275479872400246e-02, +9.95719412471739673e-01,
    };
    static const double poles[] = {
      -5.02108253128409412e-02, +2.89774652464423688e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.43245238094737126e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.79192220162268123e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.20807779837731821e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     30439.516593942277,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.91952475669608968e-01, +0.00000000000000000e+00, /* pole */
      -8.15297560941167898e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.80985929337916085e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     30439.516593942277,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -8.94365806116883988e-01, +4.47336344207461267e-01,
    };
    static const double poles[] = {
      -2.91952475669608968e-01, +0.00000000000000000e+00, /* pole */
      -8.15297560941167898e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.19014070662083915e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      43199.999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.63271264002680283e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      43199.999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.36728735997319772e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.73894589129251720e-01, +3.63892531037496358e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.31106439916625961e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.73894589129251720e-01, +3.63892531037496358e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.05001029045877625e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     61188.145497689256,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.29018906183446752e-01, +6.45645891287237039e-01, /* pole */
      +3.77264451989857252e-01, +6.02222467392582450e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.58039328153201902e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     61188.145497689256,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.13822994416502177e-01, +9.93501044761433971e-01,
      +1.13822994416502177e-01, +9.93501044761433971e-01,
    };
    static const double poles[] = {
      -2.29018906183446752e-01, +6.45645891287237039e-01, /* pole */
      +3.77264451989857252e-01, +6.02222467392582450e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.60461097729470303e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.84763688675620652e-01, +4.02092143672083402e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.91335772501768597e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.84763688675620652e-01, +4.02092143672083402e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.06572083826147945e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     26435.40044975268,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.90902470686577730e-01, +4.54763094852988492e-01, /* pole */
      -3.14235249867312272e-01, +6.63602230102455115e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.27652299938871766e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     26435.40044975268,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.61759069488428886e-01, +7.49716569077814143e-01,
      -6.61759069488428886e-01, +7.49716569077814143e-01,
    };
    static const double poles[] = {
      -6.90902470686577730e-01, +4.54763094852988492e-01, /* pole */
      -3.14235249867312272e-01, +6.63602230102455115e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.01652944416764535e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.80509037900358527e-01, +1.79324230971218773e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.00592403464569835e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.80509037900358527e-01, +1.79324230971218773e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.00833655642113085e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +4.18498893719413012e-01, +4.98843026314932103e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.95329963572531951e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +4.18498893719413012e-01, +4.98843026314932103e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.74452692590159453e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     5954.338759550481,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.93455493096058920e-02, +2.92052103912920524e-01, /* pole */
      -5.33100349706419729e-01, +5.63592138783849284e-01, /* pole */
      +4.31783141266105674e-01, +6.15439693623290429e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.26213850727657617e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     5954.338759550481,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -9.07312952019237312e-02, +9.95875409913801080e-01,
      -9.07312952019237312e-02, +9.95875409913801080e-01,
      -9.07312952019237312e-02, +9.95875409913801080e-01,
    };
    static const double poles[] = {
      -4.93455493096058920e-02, +2.92052103912920524e-01, /* pole */
      -5.33100349706419729e-01, +5.63592138783849284e-01, /* pole */
      +4.31783141266105674e-01, +6.15439693623290429e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.13943265364287677e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -2.09428042240883155e-01, +5.58199478050222853e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.56915601248463354e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -2.09428042240883155e-01, +5.58199478050222853e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.85311609239270525e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     39913.808861461504,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.86023224673108656e-01, +4.60862333716873795e-01, /* pole */
      -7.56623624334773481e-01, +4.56458027041781345e-01, /* pole */
      -2.90532397936105635e-01, +7.46448939977202630e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.61830551150999714e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     39913.808861461504,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.71018880654365435e-01, +7.41440261791442290e-01,
      -6.71018880654365435e-01, +7.41440261791442290e-01,
      -6.71018880654365435e-01, +7.41440261791442290e-01,
    };
    static const double poles[] = {
      -4.86023224673108656e-01, +4.60862333716873795e-01, /* pole */
      -7.56623624334773481e-01, +4.56458027041781345e-01, /* pole */
      -2.90532397936105635e-01, +7.46448939977202630e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.74795720675710187e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360788e-01, +0.00000000000000000e+00, /* pole */
      -8.23776107851995509e-01, +2.31801297246200161e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.29440722639082328e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360788e-01, +0.00000000000000000e+00, /* pole */
      -8.23776107851995509e-01, +2.31801297246200161e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.89819463372144315e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.36370455595763562e-01, +1.77172561180955423e-01, /* pole */
      +4.48828970018322282e-01, +5.70735893653000592e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.85630106268971778e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.36370455595763562e-01, +1.77172561180955423e-01, /* pole */
      +4.48828970018322282e-01, +5.70735893653000592e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.75413288072304363e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     38216.748969049229,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.19474444865483198e-01, +1.67758045223614372e-01, /* pole */
      +2.48989735294404574e-01, +2.49770363221124475e-01, /* pole */
      -8.08348318885111050e-01, +3.44312893633599570e-01, /* pole */
      +4.55907524496128091e-01, +6.18501954735949488e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.40902570539989502e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     38216.748969049229,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -4.17373157217609647e-01, +9.08735191149877553e-01,
      -4.17373157217609647e-01, +9.08735191149877553e-01,
      -4.17373157217609647e-01, +9.08735191149877553e-01,
      -4.17373157217609647e-01, +9.08735191149877553e-01,
    };
    static const double poles[] = {
      -6.19474444865483198e-01, +1.67758045223614372e-01, /* pole */
      +2.48989735294404574e-01, +2.49770363221124475e-01, /* pole */
      -8.08348318885111050e-01, +3.44312893633599570e-01, /* pole */
      +4.55907524496128091e-01, +6.18501954735949488e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.91304865620973785e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.64487838685476451e-01, +1.93730239872415527e-01, /* pole */
      -2.26559760326192378e-01, +6.44202022477557090e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.67179268608489917e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.64487838685476451e-01, +1.93730239872415527e-01, /* pole */
      -2.26559760326192378e-01, +6.44202022477557090e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.65829066364436689e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     21302.334162862753,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.77965660012204041e-01, +2.69132874253450449e-01, /* pole */
      -3.68830714363300227e-01, +4.03344077421924407e-01, /* pole */
      -8.62214212576799865e-01, +3.18904236917106565e-01, /* pole */
      -2.74894721370224415e-01, +7.48825788401726644e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.61811893048955623e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     21302.334162862753,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.70492002220483196e-01, +6.37449664298500474e-01,
      -7.70492002220483196e-01, +6.37449664298500474e-01,
      -7.70492002220483196e-01, +6.37449664298500474e-01,
      -7.70492002220483196e-01, +6.37449664298500474e-01,
    };
    static const double poles[] = {
      -6.77965660012204041e-01, +2.69132874253450449e-01, /* pole */
      -3.68830714363300227e-01, +4.03344077421924407e-01, /* pole */
      -8.62214212576799865e-01, +3.18904236917106565e-01, /* pole */
      -2.74894721370224415e-01, +7.48825788401726644e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.91905983648564904e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.39837108465596582e-01, +9.19923704431719891e-02, /* pole */
      -8.50482165971762849e-01, +2.55303398291673145e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.62015837202617474e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.39837108465596582e-01, +9.19923704431719891e-02, /* pole */
      -8.50482165971762849e-01, +2.55303398291673145e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.16599204406606360e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.55262758270301404e-01, +2.87413608909192708e-01, /* pole */
      +4.70228201833978510e-01, +6.15536707435050623e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.93319613014260414e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.55262758270301404e-01, +2.87413608909192708e-01, /* pole */
      +4.70228201833978454e-01, +6.15536707435050734e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.01885013869886415e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     497463.98014957784,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.84169994817681237e-01, +0.00000000000000000e+00, /* pole */
      -8.38047224264874324e-01, +0.00000000000000000e+00, /* pole */
      -8.70785384138614948e-01, +9.27853160341671063e-02, /* pole */
      +3.34047421206276784e-01, +3.14283149297108599e-01, /* pole */
      -9.43396906940308533e-01, +1.47556811789613190e-01, /* pole */
      +4.73223241532672034e-01, +6.31323765582482710e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.48245394680078579e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     497463.98014957784,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.27014090458544260e-01, +6.86622539882529415e-01,
      -7.27014090458544260e-01, +6.86622539882529415e-01,
      -7.27014090458544260e-01, +6.86622539882529415e-01,
      -7.27014090458544260e-01, +6.86622539882529415e-01,
      -7.27014090458544260e-01, +6.86622539882529415e-01,
    };
    static const double poles[] = {
      +2.84169994817681237e-01, +0.00000000000000000e+00, /* pole */
      -8.38047224264874324e-01, +0.00000000000000000e+00, /* pole */
      -8.70785384138614948e-01, +9.27853160341671063e-02, /* pole */
      +3.34047421206276784e-01, +3.14283149297108599e-01, /* pole */
      -9.43396906940308533e-01, +1.47556811789613190e-01, /* pole */
      +4.73223241532672090e-01, +6.31323765582482710e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.30759135923220436e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.08373702587479970e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.19396206884641676e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     28153.675338928784,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.14349935017134996e-01, +3.09543273937381291e-01, /* pole */
      -7.31098857299889660e-01, +3.05060951848100392e-01, /* pole */
      -3.22295826406164776e-01, +5.15961446134049262e-01, /* pole */
      -8.67521358068485959e-01, +3.45404250898759646e-01, /* pole */
      -2.78315400906743060e-01, +7.92788332225960901e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.21730632730634750e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     28153.675338928784,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.56189626022273353e-01, +6.54352542209697741e-01,
      -7.56189626022273353e-01, +6.54352542209697741e-01,
      -7.56189626022273353e-01, +6.54352542209697741e-01,
      -7.56189626022273353e-01, +6.54352542209697741e-01,
      -7.56189626022273353e-01, +6.54352542209697741e-01,
    };
    static const double poles[] = {
      -5.14349935017134996e-01, +3.09543273937381291e-01, /* pole */
      -7.31098857299889660e-01, +3.05060951848100392e-01, /* pole */
      -3.22295826406164776e-01, +5.15961446134049262e-01, /* pole */
      -8.67521358068485959e-01, +3.45404250898759591e-01, /* pole */
      -2.78315400906743005e-01, +7.92788332225960901e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.27039743057118409e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      21599.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.60845213036122403e-01, +1.45308505601072363e-01, /* pole */
      -8.68155082767364417e-01, +2.68274674328105200e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.99940204219629347e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      21599.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.60845213036122403e-01, +1.45308505601072363e-01, /* pole */
      -8.68155082767364528e-01, +2.68274674328105200e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.97957803700199797e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.29947580576855581e-01, +1.17538498363248825e-01, /* pole */
      +3.73894589129251720e-01, +3.63892531037496358e-01, /* pole */
      +4.86018352571280066e-01, +6.46153061525396866e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.58506418423726757e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.29947580576855581e-01, +1.17538498363248825e-01, /* pole */
      +3.73894589129251720e-01, +3.63892531037496358e-01, /* pole */
      +4.86018352571280010e-01, +6.46153061525396866e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.47732499549473567e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1800,
      /* passband_edge2 = */     3334.3723287892644,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.74698541628616405e-02, +6.35733099719693495e-01, /* pole */
      +2.56626960580526697e-01, +6.08908138396950904e-01, /* pole */
      -5.36761802550585407e-02, +7.36737184854158400e-01, /* pole */
      +4.10115410958564575e-01, +6.54402689476937804e-01, /* pole */
      -1.47734228440827703e-01, +8.89675846187781727e-01, /* pole */
      +5.34722261876959037e-01, +7.46615246926171094e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.17724405307388603e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1800,
      /* passband_edge2 = */     3334.3723287892644,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +2.44118172847733461e-01, +9.69745491191108422e-01,
      +2.44118172847733461e-01, +9.69745491191108422e-01,
      +2.44118172847733461e-01, +9.69745491191108422e-01,
      +2.44118172847733461e-01, +9.69745491191108422e-01,
      +2.44118172847733461e-01, +9.69745491191108422e-01,
      +2.44118172847733461e-01, +9.69745491191108422e-01,
    };
    static const double poles[] = {
      +8.74698541628616405e-02, +6.35733099719693495e-01, /* pole */
      +2.56626960580526697e-01, +6.08908138396950904e-01, /* pole */
      -5.36761802550585407e-02, +7.36737184854158400e-01, /* pole */
      +4.10115410958564575e-01, +6.54402689476937804e-01, /* pole */
      -1.47734228440827731e-01, +8.89675846187781616e-01, /* pole */
      +5.34722261876959037e-01, +7.46615246926171094e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.00347893071856464e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.61059591955040049e-01, +1.28294130142620144e-01, /* pole */
      -1.84763688675620652e-01, +4.02092143672083402e-01, /* pole */
      -2.47977059457146765e-01, +7.37189677430886592e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.01154134924536071e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.61059591955040049e-01, +1.28294130142620144e-01, /* pole */
      -1.84763688675620652e-01, +4.02092143672083402e-01, /* pole */
      -2.47977059457146903e-01, +7.37189677430886592e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.03128747626644052e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     1818.2740087597688,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.78470231900404896e-01, +1.83489198608382975e-01, /* pole */
      -3.87056170947889189e-01, +2.79888152000017210e-01, /* pole */
      -8.16162646479227316e-01, +2.34086386851546702e-01, /* pole */
      -2.83712646452621675e-01, +5.32984822516309409e-01, /* pole */
      -9.13010474468688105e-01, +2.67346914788777867e-01, /* pole */
      -2.74974474964782278e-01, +8.00154058179036731e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.99201195938670911e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              6,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     1818.2740087597688,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -8.10926010569858691e-01, +5.85148703648272206e-01,
      -8.10926010569858691e-01, +5.85148703648272206e-01,
      -8.10926010569858691e-01, +5.85148703648272206e-01,
      -8.10926010569858691e-01, +5.85148703648272206e-01,
      -8.10926010569858691e-01, +5.85148703648272206e-01,
      -8.10926010569858691e-01, +5.85148703648272206e-01,
    };
    static const double poles[] = {
      -6.78470231900404896e-01, +1.83489198608382975e-01, /* pole */
      -3.87056170947889189e-01, +2.79888152000017210e-01, /* pole */
      -8.16162646479227316e-01, +2.34086386851546702e-01, /* pole */
      -2.83712646452621675e-01, +5.32984822516309409e-01, /* pole */
      -9.13010474468688216e-01, +2.67346914788777867e-01, /* pole */
      -2.74974474964782167e-01, +8.00154058179036731e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.38500485056616773e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      44.999999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.37035436954588064e-01, +8.19060708284092831e-02, /* pole */
      -7.69014763633104170e-01, +1.60612082353038588e-01, /* pole */
      -8.23776107851995065e-01, +2.31801297246200494e-01, /* pole */
      -9.02621625120219817e-01, +2.88823968974642353e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.02734998169869463e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      44.999999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.37035436954588064e-01, +8.19060708284093109e-02, /* pole */
      -7.69014763633104170e-01, +1.60612082353038615e-01, /* pole */
      -8.23776107851995065e-01, +2.31801297246200494e-01, /* pole */
      -9.02621625120219817e-01, +2.88823968974642353e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.52599217264088717e-08;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 72000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      10800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.25290347990102147e-01, +3.19402469005045028e-02, /* pole */
      +3.28274740316640889e-01, +9.60436760940993617e-02, /* pole */
      +3.34347266094354778e-01, +1.60819757334291907e-01, /* pole */
      +3.43722564396382602e-01, +2.26729591662073182e-01, /* pole */
      +3.56741254744752434e-01, +2.94252653425110644e-01, /* pole */
      +3.73894589129251775e-01, +3.63892531037496414e-01, /* pole */
      +3.95861417371774549e-01, +4.36180096940640860e-01, /* pole */
      +4.23561675865430531e-01, +5.11671608148781987e-01, /* pole */
      +4.58232603326535426e-01, +5.90937068544945987e-01, /* pole */
      +5.01536547951508926e-01, +6.74530185240849400e-01, /* pole */
      +5.55712520051447001e-01, +7.62923857688642792e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.45413139953903786e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 72000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      10800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.25290347990102147e-01, +3.19402469005045028e-02, /* pole */
      +3.28274740316640889e-01, +9.60436760940993617e-02, /* pole */
      +3.34347266094354778e-01, +1.60819757334291907e-01, /* pole */
      +3.43722564396382602e-01, +2.26729591662073154e-01, /* pole */
      +3.56741254744752434e-01, +2.94252653425110644e-01, /* pole */
      +3.73894589129251775e-01, +3.63892531037496414e-01, /* pole */
      +3.95861417371774604e-01, +4.36180096940640805e-01, /* pole */
      +4.23561675865430587e-01, +5.11671608148781987e-01, /* pole */
      +4.58232603326535426e-01, +5.90937068544945987e-01, /* pole */
      +5.01536547951509037e-01, +6.74530185240849400e-01, /* pole */
      +5.55712520051447001e-01, +7.62923857688642792e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.56509917634426046e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 72000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      10800,
      /* passband_edge2 = */     28499.95470626263,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.28500561250078588e-01, +1.34122844596263863e-01, /* pole */
      +2.55533782808165834e-02, +1.41276102683642990e-01, /* pole */
      -3.27641145433853920e-01, +2.22055221093609090e-01, /* pole */
      +1.24666997286851572e-01, +2.43588233524703796e-01, /* pole */
      -3.95806167800714481e-01, +2.82178081188899976e-01, /* pole */
      +1.92776971109253487e-01, +3.18313586206561805e-01, /* pole */
      -4.52091539277116650e-01, +3.31033958557741537e-01, /* pole */
      +2.48976874380060886e-01, +3.82153534256430838e-01, /* pole */
      -5.02061046393196486e-01, +3.73711426442690509e-01, /* pole */
      +2.98826723787816717e-01, +4.40371993930194605e-01, /* pole */
      -5.48369239339572934e-01, +4.12649418025681058e-01, /* pole */
      +3.44975500136289581e-01, +4.95608469057411294e-01, /* pole */
      -5.92617844316749132e-01, +4.49297204834579800e-01, /* pole */
      +3.89016980699895432e-01, +5.49549290845275373e-01, /* pole */
      -6.35945667967635964e-01, +4.84653229411491693e-01, /* pole */
      +4.32078740265993522e-01, +6.03481683776340705e-01, /* pole */
      -6.79281340204385575e-01, +5.19495296499098091e-01, /* pole */
      +4.75073478467344157e-01, +6.58545386358707296e-01, /* pole */
      -7.23480714415179249e-01, +5.54501453088236063e-01, /* pole */
      +5.18834159942495154e-01, +7.15884757111988734e-01, /* pole */
      -7.69423151856730381e-01, +5.90328254840993050e-01, /* pole */
      +5.64206589013213300e-01, +7.76774127163727068e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.99457235929087803e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              22,
      /* sampling_frequency = */ 72000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      10800,
      /* passband_edge2 = */     28499.95470626263,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
      -2.00321095307886110e-01, +9.79730298997968596e-01,
    };
    static const double poles[] = {
      -2.28500561250078588e-01, +1.34122844596263863e-01, /* pole */
      +2.55533782808165834e-02, +1.41276102683642990e-01, /* pole */
      -3.27641145433853920e-01, +2.22055221093609090e-01, /* pole */
      +1.24666997286851572e-01, +2.43588233524703796e-01, /* pole */
      -3.95806167800714481e-01, +2.82178081188899976e-01, /* pole */
      +1.92776971109253487e-01, +3.18313586206561805e-01, /* pole */
      -4.52091539277116650e-01, +3.31033958557741592e-01, /* pole */
      +2.48976874380060886e-01, +3.82153534256430893e-01, /* pole */
      -5.02061046393196486e-01, +3.73711426442690509e-01, /* pole */
      +2.98826723787816717e-01, +4.40371993930194605e-01, /* pole */
      -5.48369239339572934e-01, +4.12649418025681058e-01, /* pole */
      +3.44975500136289581e-01, +4.95608469057411294e-01, /* pole */
      -5.92617844316749021e-01, +4.49297204834579911e-01, /* pole */
      +3.89016980699895376e-01, +5.49549290845275373e-01, /* pole */
      -6.35945667967635964e-01, +4.84653229411491637e-01, /* pole */
      +4.32078740265993522e-01, +6.03481683776340705e-01, /* pole */
      -6.79281340204385575e-01, +5.19495296499098091e-01, /* pole */
      +4.75073478467344157e-01, +6.58545386358707296e-01, /* pole */
      -7.23480714415179138e-01, +5.54501453088236063e-01, /* pole */
      +5.18834159942495043e-01, +7.15884757111988734e-01, /* pole */
      -7.69423151856730381e-01, +5.90328254840993050e-01, /* pole */
      +5.64206589013213300e-01, +7.76774127163727068e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.54855726173966059e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      300,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.08373702587479970e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      300,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.19396206884641676e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      300,
      /* passband_edge2 = */     419.44657581014218,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.90455946556222655e-01, +4.40746386999675333e-01, /* pole */
      -6.70972130150551016e-01, +4.16071159994052198e-01, /* pole */
      -3.34890997880145258e-01, +5.92683914177025040e-01, /* pole */
      -8.11108193780845421e-01, +4.54057770526997584e-01, /* pole */
      -2.85503587359147992e-01, +8.20308041360828910e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.71407163282520172e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      300,
      /* passband_edge2 = */     419.44657581014218,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.83655475346808039e-01, +7.29804899290440456e-01,
      -6.83655475346808039e-01, +7.29804899290440456e-01,
      -6.83655475346808039e-01, +7.29804899290440456e-01,
      -6.83655475346808039e-01, +7.29804899290440456e-01,
      -6.83655475346808039e-01, +7.29804899290440456e-01,
    };
    static const double poles[] = {
      -4.90455946556222655e-01, +4.40746386999675333e-01, /* pole */
      -6.70972130150551016e-01, +4.16071159994052198e-01, /* pole */
      -3.34890997880145258e-01, +5.92683914177025040e-01, /* pole */
      -8.11108193780845532e-01, +4.54057770526997695e-01, /* pole */
      -2.85503587359147881e-01, +8.20308041360829132e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.86148197308269014e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              11,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      25199.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.33557103099135799e-01, +6.71501522976577364e-02, /* pole */
      -7.54829740401174565e-01, +1.32597053258062175e-01, /* pole */
      -7.90989443759871125e-01, +1.94233724234329941e-01, /* pole */
      -8.42858516712523431e-01, +2.49113030985130157e-01, /* pole */
      -9.10993130442793397e-01, +2.92986761967777121e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.29638801642986090e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              11,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      25199.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.33557103099135799e-01, +6.71501522976577364e-02, /* pole */
      -7.54829740401174565e-01, +1.32597053258062175e-01, /* pole */
      -7.90989443759871125e-01, +1.94233724234329941e-01, /* pole */
      -8.42858516712523542e-01, +2.49113030985130129e-01, /* pole */
      -9.10993130442793508e-01, +2.92986761967777065e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.18651903664972026e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              25,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      6615,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26069552067423885e-01, +5.62490835046795257e-02, /* pole */
      +3.29549884458075648e-01, +1.12802376216117425e-01, /* pole */
      +3.35454618411899719e-01, +1.69968138944206093e-01, /* pole */
      +3.43945864627163811e-01, +2.28062608297865177e-01, /* pole */
      +3.55262758270301404e-01, +2.87413608909192708e-01, /* pole */
      +3.69734856941504986e-01, +3.48363538986663945e-01, /* pole */
      +3.87801250662635100e-01, +4.11271175027993430e-01, /* pole */
      +4.10037062747787617e-01, +4.76511324834964778e-01, /* pole */
      +4.37189711431299810e-01, +5.44470643788568021e-01, /* pole */
      +4.70228201833978399e-01, +6.15536707435050512e-01, /* pole */
      +5.10409841815224641e-01, +6.90075337555246748e-01, /* pole */
      +5.59370038897379018e-01, +7.68387573173730298e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.77275246021120981e-11;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              25,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      6615,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26069552067423940e-01, +5.62490835046795326e-02, /* pole */
      +3.29549884458075537e-01, +1.12802376216117439e-01, /* pole */
      +3.35454618411899608e-01, +1.69968138944206093e-01, /* pole */
      +3.43945864627163811e-01, +2.28062608297865177e-01, /* pole */
      +3.55262758270301404e-01, +2.87413608909192708e-01, /* pole */
      +3.69734856941504986e-01, +3.48363538986663945e-01, /* pole */
      +3.87801250662635100e-01, +4.11271175027993430e-01, /* pole */
      +4.10037062747787617e-01, +4.76511324834964778e-01, /* pole */
      +4.37189711431299810e-01, +5.44470643788568021e-01, /* pole */
      +4.70228201833978454e-01, +6.15536707435050401e-01, /* pole */
      +5.10409841815224641e-01, +6.90075337555246748e-01, /* pole */
      +5.59370038897379018e-01, +7.68387573173730298e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.71108650178311674e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              25,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      6615,
      /* passband_edge2 = */     18246.451691005739,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +1.10302179436425785e-01, +0.00000000000000000e+00, /* pole */
      -3.91916649905401204e-01, +0.00000000000000000e+00, /* pole */
      -4.14976916961702191e-01, +1.00524179391328586e-01, /* pole */
      +1.33412716984050272e-01, +1.19007645608396939e-01, /* pole */
      -4.56810956588564798e-01, +1.72626008321890040e-01, /* pole */
      +1.75399164908664140e-01, +2.09739014485113923e-01, /* pole */
      -4.99122310242019307e-01, +2.26498446684061799e-01, /* pole */
      +2.17969963800983768e-01, +2.82537781054530368e-01, /* pole */
      -5.39329618966993274e-01, +2.70174370275835185e-01, /* pole */
      +2.58552294978210473e-01, +3.45597133095265741e-01, /* pole */
      -5.77613403891093946e-01, +3.07584313737284676e-01, /* pole */
      +2.97339470829475594e-01, +4.03023140317239625e-01, /* pole */
      -6.14525728409080818e-01, +3.40849951662788142e-01, /* pole */
      +3.34901554441042804e-01, +4.57134964712520075e-01, /* pole */
      -6.50619864704196638e-01, +3.71252468923224388e-01, /* pole */
      +3.71816458510963788e-01, +5.09441639381726596e-01, /* pole */
      -6.86405393236309891e-01, +3.99638995978753520e-01, /* pole */
      +4.08627157217723691e-01, +5.61059396342188599e-01, /* pole */
      -7.22362453391616866e-01, +4.26612780824403293e-01, /* pole */
      +4.45859058957181253e-01, +6.12916386390598755e-01, /* pole */
      -7.58966737349850318e-01, +4.52631576392602486e-01, /* pole */
      +4.84049669605990496e-01, +6.65871032447745259e-01, /* pole */
      -7.96716990805509395e-01, +4.78062892864697275e-01, /* pole */
      +5.23783206274811119e-01, +7.20795398641253304e-01, /* pole */
      -8.36165657609229562e-01, +5.03216987413633743e-01, /* pole */
      +5.65732335968218814e-01, +7.78648812421340875e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.32829452730899699e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              25,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      6615,
      /* passband_edge2 = */     18246.451691005739,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
      -2.94338506482689222e-01, +9.55701231348761948e-01,
    };
    static const double poles[] = {
      +1.10302179436425785e-01, +0.00000000000000000e+00, /* pole */
      -3.91916649905401204e-01, +0.00000000000000000e+00, /* pole */
      -4.14976916961702191e-01, +1.00524179391328586e-01, /* pole */
      +1.33412716984050272e-01, +1.19007645608396939e-01, /* pole */
      -4.56810956588564798e-01, +1.72626008321889929e-01, /* pole */
      +1.75399164908664196e-01, +2.09739014485113812e-01, /* pole */
      -4.99122310242019362e-01, +2.26498446684061716e-01, /* pole */
      +2.17969963800983879e-01, +2.82537781054530257e-01, /* pole */
      -5.39329618966993274e-01, +2.70174370275835185e-01, /* pole */
      +2.58552294978210473e-01, +3.45597133095265741e-01, /* pole */
      -5.77613403891093946e-01, +3.07584313737284676e-01, /* pole */
      +2.97339470829475594e-01, +4.03023140317239625e-01, /* pole */
      -6.14525728409080818e-01, +3.40849951662788142e-01, /* pole */
      +3.34901554441042804e-01, +4.57134964712520075e-01, /* pole */
      -6.50619864704196638e-01, +3.71252468923224388e-01, /* pole */
      +3.71816458510963788e-01, +5.09441639381726596e-01, /* pole */
      -6.86405393236309891e-01, +3.99638995978753520e-01, /* pole */
      +4.08627157217723691e-01, +5.61059396342188599e-01, /* pole */
      -7.22362453391616866e-01, +4.26612780824403293e-01, /* pole */
      +4.45859058957181253e-01, +6.12916386390598755e-01, /* pole */
      -7.58966737349850318e-01, +4.52631576392602486e-01, /* pole */
      +4.84049669605990496e-01, +6.65871032447745259e-01, /* pole */
      -7.96716990805509395e-01, +4.78062892864697275e-01, /* pole */
      +5.23783206274811119e-01, +7.20795398641253304e-01, /* pole */
      -8.36165657609229562e-01, +5.03216987413633743e-01, /* pole */
      +5.65732335968218814e-01, +7.78648812421340875e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.52755731467166356e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              15,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.60089733620291885e-01, +1.02439241404746403e-01, /* pole */
      -1.65352888555580280e-01, +2.06989848857221548e-01, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -1.88841712260386513e-01, +4.31912078361670559e-01, /* pole */
      -2.09428042240883155e-01, +5.58199478050222853e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
      -2.81074665864245676e-01, +8.60319988834840510e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.35930924385637363e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              15,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.60089733620291857e-01, +1.02439241404746431e-01, /* pole */
      -1.65352888555580335e-01, +2.06989848857221548e-01, /* pole */
      -1.74643012930345315e-01, +3.15932178339838277e-01, /* pole */
      -1.88841712260386541e-01, +4.31912078361670504e-01, /* pole */
      -2.09428042240883155e-01, +5.58199478050222853e-01, /* pole */
      -2.38827386547005638e-01, +6.99059936589549347e-01, /* pole */
      -2.81074665864245787e-01, +8.60319988834840510e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.12780465012447030e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              15,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1523.4519172705125,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.33571258403175319e-01, +1.25693198643327980e-01, /* pole */
      -6.72375408169815447e-01, +1.55627213963755923e-01, /* pole */
      -3.97242589844570948e-01, +2.34006116530847408e-01, /* pole */
      -7.41545205059669255e-01, +1.85772717455410769e-01, /* pole */
      -3.35690608494169429e-01, +3.43794984033497897e-01, /* pole */
      -7.92356757209380591e-01, +2.07286986591309452e-01, /* pole */
      -2.98243367923950953e-01, +4.47539230831064505e-01, /* pole */
      -8.34949270966690138e-01, +2.24098807827112501e-01, /* pole */
      -2.75876347993284587e-01, +5.50604003019450783e-01, /* pole */
      -8.73313678187574438e-01, +2.38081733205705498e-01, /* pole */
      -2.66414656038687081e-01, +6.56462932482270944e-01, /* pole */
      -9.09673719691592830e-01, +2.50253301872091771e-01, /* pole */
      -2.70487862773000409e-01, +7.67920245136072155e-01, /* pole */
      -9.45611969775854933e-01, +2.61232985161140185e-01, /* pole */
      -2.90981256567184932e-01, +8.87508908329875279e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.18793122578616299e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              15,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1523.4519172705125,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
      -8.20565107820349415e-01, +5.71553063002708805e-01,
    };
    static const double poles[] = {
      -5.33571258403175319e-01, +1.25693198643327980e-01, /* pole */
      -6.72375408169815558e-01, +1.55627213963756089e-01, /* pole */
      -3.97242589844571170e-01, +2.34006116530847602e-01, /* pole */
      -7.41545205059669144e-01, +1.85772717455410796e-01, /* pole */
      -3.35690608494169374e-01, +3.43794984033497897e-01, /* pole */
      -7.92356757209380591e-01, +2.07286986591309452e-01, /* pole */
      -2.98243367923950953e-01, +4.47539230831064505e-01, /* pole */
      -8.34949270966690027e-01, +2.24098807827112473e-01, /* pole */
      -2.75876347993284532e-01, +5.50604003019450672e-01, /* pole */
      -8.73313678187574438e-01, +2.38081733205705498e-01, /* pole */
      -2.66414656038687081e-01, +6.56462932482270944e-01, /* pole */
      -9.09673719691592719e-01, +2.50253301872091716e-01, /* pole */
      -2.70487862773000354e-01, +7.67920245136072155e-01, /* pole */
      -9.45611969775854933e-01, +2.61232985161140130e-01, /* pole */
      -2.90981256567184876e-01, +8.87508908329875168e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.78179443895965059e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.28660297941478352e-01, +3.70368111365830116e-02, /* pole */
      -7.45730065186936075e-01, +1.10002981422759619e-01, /* pole */
      -7.80509037900358527e-01, +1.79324230971218773e-01, /* pole */
      -8.34047361622782812e-01, +2.41461356150367262e-01, /* pole */
      -9.07201568548007264e-01, +2.91138579064851410e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.64373550347606989e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.28660297941478352e-01, +3.70368111365830255e-02, /* pole */
      -7.45730065186936075e-01, +1.10002981422759619e-01, /* pole */
      -7.80509037900358527e-01, +1.79324230971218773e-01, /* pole */
      -8.34047361622782812e-01, +2.41461356150367207e-01, /* pole */
      -9.07201568548007264e-01, +2.91138579064851466e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.61969508002029511e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26913676960878308e-01, +7.40606649197373956e-02, /* pole */
      +3.32988427444231183e-01, +1.48816017610529067e-01, /* pole */
      +3.43430960565199894e-01, +2.24976603665544561e-01, /* pole */
      +3.58749497993094857e-01, +3.03283694821467387e-01, /* pole */
      +3.79722841724041915e-01, +3.84521433111573219e-01, /* pole */
      +4.07479502563121987e-01, +4.69522659687195532e-01, /* pole */
      +4.43618734580560670e-01, +5.59160941959820690e-01, /* pole */
      +4.90392378908589333e-01, +6.54313387316398987e-01, /* pole */
      +5.50975621266036497e-01, +7.55762724153150156e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.72899928986409523e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26913676960878308e-01, +7.40606649197374095e-02, /* pole */
      +3.32988427444231183e-01, +1.48816017610529094e-01, /* pole */
      +3.43430960565199894e-01, +2.24976603665544561e-01, /* pole */
      +3.58749497993094857e-01, +3.03283694821467387e-01, /* pole */
      +3.79722841724041915e-01, +3.84521433111573219e-01, /* pole */
      +4.07479502563121987e-01, +4.69522659687195532e-01, /* pole */
      +4.43618734580560670e-01, +5.59160941959820690e-01, /* pole */
      +4.90392378908589333e-01, +6.54313387316398987e-01, /* pole */
      +5.50975621266036497e-01, +7.55762724153150156e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.46489683728376126e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     163580.42317170318,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.98892513258352333e-02, +5.03557635373141688e-01, /* pole */
      -5.09538479442705160e-03, +5.15044497482759489e-01, /* pole */
      +1.45057743566352426e-01, +5.06459578941420685e-01, /* pole */
      -7.50948084538120009e-02, +5.39127002430498381e-01, /* pole */
      +2.15617896667297387e-01, +5.21862642659524201e-01, /* pole */
      -1.38033155818198416e-01, +5.72183452034085827e-01, /* pole */
      +2.79522270784356541e-01, +5.46047140089379379e-01, /* pole */
      -1.94145869176879649e-01, +6.11292666101678206e-01, /* pole */
      +3.37056977149398418e-01, +5.75986811552318678e-01, /* pole */
      -2.44508211956768390e-01, +6.54885547660666312e-01, /* pole */
      +3.89375735194783690e-01, +6.09996467015380683e-01, /* pole */
      -2.90214605050809260e-01, +7.02428429564005863e-01, /* pole */
      +4.37688319453224672e-01, +6.47411049800756477e-01, /* pole */
      -3.32113211778506223e-01, +7.54065171844922943e-01, /* pole */
      +4.83009585578212830e-01, +6.88223219404069275e-01, /* pole */
      -3.70744425142380096e-01, +8.10419397952705789e-01, /* pole */
      +5.26120727589572179e-01, +7.32881454689536249e-01, /* pole */
      -4.06309208394863675e-01, +8.72527255962239012e-01, /* pole */
      +5.67573805803229803e-01, +7.82220862810328321e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.40920010891226749e-08;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              19,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     163580.42317170318,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
      +1.11071532085875280e-01, +9.93812414271474265e-01,
    };
    static const double poles[] = {
      +6.98892513258352333e-02, +5.03557635373141688e-01, /* pole */
      -5.09538479442706461e-03, +5.15044497482759489e-01, /* pole */
      +1.45057743566352454e-01, +5.06459578941420685e-01, /* pole */
      -7.50948084538120425e-02, +5.39127002430498381e-01, /* pole */
      +2.15617896667297415e-01, +5.21862642659524201e-01, /* pole */
      -1.38033155818198416e-01, +5.72183452034085827e-01, /* pole */
      +2.79522270784356541e-01, +5.46047140089379379e-01, /* pole */
      -1.94145869176879649e-01, +6.11292666101678206e-01, /* pole */
      +3.37056977149398418e-01, +5.75986811552318678e-01, /* pole */
      -2.44508211956768390e-01, +6.54885547660666312e-01, /* pole */
      +3.89375735194783690e-01, +6.09996467015380683e-01, /* pole */
      -2.90214605050809260e-01, +7.02428429564005863e-01, /* pole */
      +4.37688319453224672e-01, +6.47411049800756477e-01, /* pole */
      -3.32113211778506223e-01, +7.54065171844922943e-01, /* pole */
      +4.83009585578212830e-01, +6.88223219404069275e-01, /* pole */
      -3.70744425142380096e-01, +8.10419397952705789e-01, /* pole */
      +5.26120727589572179e-01, +7.32881454689536249e-01, /* pole */
      -4.06309208394863675e-01, +8.72527255962239012e-01, /* pole */
      +5.67573805803229803e-01, +7.82220862810328321e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.02013977043604502e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.59251484074363681e-01, +7.30494406881029429e-02, /* pole */
      -1.61890396217426707e-01, +1.46861001428221033e-01, /* pole */
      -1.66418009598821526e-01, +2.22227427571765862e-01, /* pole */
      -1.73041233126615318e-01, +3.00005197045403349e-01, /* pole */
      -1.82077405243711826e-01, +3.81152908385300526e-01, /* pole */
      -1.93987464220118200e-01, +4.66778433224661227e-01, /* pole */
      -2.09428042240883183e-01, +5.58199478050222964e-01, /* pole */
      -2.29332969523399488e-01, +6.57024007787231357e-01, /* pole */
      -2.55042379597360203e-01, +7.65259655051827692e-01, /* pole */
      -2.88511732046359826e-01, +8.85464925837302297e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.75244137951506205e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58384440324536330e-01, +0.00000000000000000e+00, /* pole */
      -1.59251484074363681e-01, +7.30494406881029429e-02, /* pole */
      -1.61890396217426763e-01, +1.46861001428221033e-01, /* pole */
      -1.66418009598821526e-01, +2.22227427571765862e-01, /* pole */
      -1.73041233126615263e-01, +3.00005197045403404e-01, /* pole */
      -1.82077405243711798e-01, +3.81152908385300582e-01, /* pole */
      -1.93987464220118200e-01, +4.66778433224661227e-01, /* pole */
      -2.09428042240883183e-01, +5.58199478050222964e-01, /* pole */
      -2.29332969523399488e-01, +6.57024007787231357e-01, /* pole */
      -2.55042379597360203e-01, +7.65259655051827692e-01, /* pole */
      -2.88511732046359826e-01, +8.85464925837302297e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.19013219412544242e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      30,
      /* passband_edge2 = */     40.409332524724881,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.71635364381355726e-01, +5.20568904376503383e-01, /* pole */
      -5.10228721694787057e-01, +5.05780383900322872e-01, /* pole */
      -4.34364310725643521e-01, +5.41539960894602324e-01, /* pole */
      -5.48854712385739019e-01, +4.96866974236190084e-01, /* pole */
      -3.99742992004848863e-01, +5.68492205528147165e-01, /* pole */
      -5.86537209896791478e-01, +4.93128439189089107e-01, /* pole */
      -3.68862498656548132e-01, +6.00824217417519568e-01, /* pole */
      -6.22725804139408257e-01, +4.93718626375213798e-01, /* pole */
      -3.42470482855824099e-01, +6.37772778164071297e-01, /* pole */
      -6.57238931441464613e-01, +4.97859772774426723e-01, /* pole */
      -3.21036568213795448e-01, +6.78615620611407788e-01, /* pole */
      -6.90137342032879353e-01, +5.04951175018011678e-01, /* pole */
      -3.04890895094370840e-01, +7.22763548527249111e-01, /* pole */
      -7.21608574495756216e-01, +5.14593383603475063e-01, /* pole */
      -2.94354863962429103e-01, +7.69760238932061025e-01, /* pole */
      -7.51888494151208620e-01, +5.26573280893911200e-01, /* pole */
      -2.89837043212099577e-01, +8.19231054904252964e-01, /* pole */
      -7.81214645155681398e-01, +5.40841303580752109e-01, /* pole */
      -2.91896980236265535e-01, +8.70806460279845207e-01, /* pole */
      -8.09798220635419397e-01, +5.57495118203683915e-01, /* pole */
      -3.01284537117194051e-01, +9.24026657497757808e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -4.36492157414775845e-12;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              21,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      30,
      /* passband_edge2 = */     40.409332524724881,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
      -6.31612816120625031e-01, +7.75283980559493768e-01,
    };
    static const double poles[] = {
      -4.71635364381355726e-01, +5.20568904376503383e-01, /* pole */
      -5.10228721694787057e-01, +5.05780383900322872e-01, /* pole */
      -4.34364310725643521e-01, +5.41539960894602324e-01, /* pole */
      -5.48854712385739019e-01, +4.96866974236190029e-01, /* pole */
      -3.99742992004848863e-01, +5.68492205528147165e-01, /* pole */
      -5.86537209896791478e-01, +4.93128439189089107e-01, /* pole */
      -3.68862498656548132e-01, +6.00824217417519568e-01, /* pole */
      -6.22725804139408257e-01, +4.93718626375213798e-01, /* pole */
      -3.42470482855824099e-01, +6.37772778164071297e-01, /* pole */
      -6.57238931441464613e-01, +4.97859772774426723e-01, /* pole */
      -3.21036568213795448e-01, +6.78615620611407788e-01, /* pole */
      -6.90137342032879353e-01, +5.04951175018011678e-01, /* pole */
      -3.04890895094370840e-01, +7.22763548527249111e-01, /* pole */
      -7.21608574495756216e-01, +5.14593383603475063e-01, /* pole */
      -2.94354863962429103e-01, +7.69760238932061025e-01, /* pole */
      -7.51888494151208620e-01, +5.26573280893911200e-01, /* pole */
      -2.89837043212099577e-01, +8.19231054904252964e-01, /* pole */
      -7.81214645155681398e-01, +5.40841303580752109e-01, /* pole */
      -2.91896980236265535e-01, +8.70806460279845207e-01, /* pole */
      -8.09798220635419397e-01, +5.57495118203683915e-01, /* pole */
      -3.01284537117194051e-01, +9.24026657497757808e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.13014417263094698e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26812450593527726e-01, +1.32413546324980352e-02, /* pole */
      -7.28973947770919617e-01, +3.96751879794383644e-02, /* pole */
      -7.33308152778690991e-01, +6.59605625641492832e-02, /* pole */
      -7.39837108465596582e-01, +9.19923704431719891e-02, /* pole */
      -7.48592896533194718e-01, +1.17656191233447588e-01, /* pole */
      -7.59616355250559705e-01, +1.42824359809151757e-01, /* pole */
      -7.72955194406324986e-01, +1.67351854887461016e-01, /* pole */
      -7.88661351438338043e-01, +1.91072002804387048e-01, /* pole */
      -8.06787388697891794e-01, +2.13792034120484531e-01, /* pole */
      -8.27381688574795437e-01, +2.35288594149516112e-01, /* pole */
      -8.50482165971762849e-01, +2.55303398291673145e-01, /* pole */
      -8.76108195650255550e-01, +2.73539345722034977e-01, /* pole */
      -9.04250460133279921e-01, +2.89657565944999529e-01, /* pole */
      -9.34858484093401443e-01, +3.03276073544077707e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.97907291843755365e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26812450593527726e-01, +1.32413546324980317e-02, /* pole */
      -7.28973947770919617e-01, +3.96751879794383644e-02, /* pole */
      -7.33308152778690991e-01, +6.59605625641492832e-02, /* pole */
      -7.39837108465596582e-01, +9.19923704431719891e-02, /* pole */
      -7.48592896533194718e-01, +1.17656191233447574e-01, /* pole */
      -7.59616355250559705e-01, +1.42824359809151757e-01, /* pole */
      -7.72955194406324986e-01, +1.67351854887461016e-01, /* pole */
      -7.88661351438338043e-01, +1.91072002804387048e-01, /* pole */
      -8.06787388697891794e-01, +2.13792034120484531e-01, /* pole */
      -8.27381688574795437e-01, +2.35288594149516112e-01, /* pole */
      -8.50482165971762849e-01, +2.55303398291673145e-01, /* pole */
      -8.76108195650255550e-01, +2.73539345722034977e-01, /* pole */
      -9.04250460133279921e-01, +2.89657565944999529e-01, /* pole */
      -9.34858484093401443e-01, +3.03276073544077707e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -9.11864941901092943e-19;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26550819654802205e-01, +6.69883348971415521e-02, /* pole */
      +3.31506209204619096e-01, +1.34490641285191503e-01, /* pole */
      +3.39976603383559950e-01, +2.03030537377514142e-01, /* pole */
      +3.52296052643801427e-01, +2.73150469767719839e-01, /* pole */
      +3.68968160192337302e-01, +3.45419674684851297e-01, /* pole */
      +3.90707154179414873e-01, +4.20439467461845628e-01, /* pole */
      +4.18498893719413123e-01, +4.98843026314932214e-01, /* pole */
      +4.53689549114993373e-01, +5.81284198827045739e-01, /* pole */
      +4.98113294499171855e-01, +6.68404856587423546e-01, /* pole */
      +5.54274944426260685e-01, +7.60760806090316377e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.29436797716207228e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.24919696232906341e-01, +0.00000000000000000e+00, /* pole */
      +3.26550819654802205e-01, +6.69883348971415521e-02, /* pole */
      +3.31506209204619040e-01, +1.34490641285191503e-01, /* pole */
      +3.39976603383559950e-01, +2.03030537377514142e-01, /* pole */
      +3.52296052643801538e-01, +2.73150469767719895e-01, /* pole */
      +3.68968160192337358e-01, +3.45419674684851241e-01, /* pole */
      +3.90707154179414873e-01, +4.20439467461845628e-01, /* pole */
      +4.18498893719413123e-01, +4.98843026314932214e-01, /* pole */
      +4.53689549114993373e-01, +5.81284198827045739e-01, /* pole */
      +4.98113294499171855e-01, +6.68404856587423546e-01, /* pole */
      +5.54274944426260685e-01, +7.60760806090316377e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.31140358434340032e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     277204.24914717983,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.06409551696236093e-01, +6.41346540112892050e-01, /* pole */
      +1.63287105300986030e-01, +6.52784727603283543e-01, /* pole */
      +2.50106006130429492e-01, +6.35910759997425856e-01, /* pole */
      +1.21762488727462523e-01, +6.70089765209590849e-01, /* pole */
      +2.93370509818869918e-01, +6.36270274215034237e-01, /* pole */
      +8.26995231900126138e-02, +6.92848504416854527e-01, /* pole */
      +3.35393782185517575e-01, +6.41942474670548768e-01, /* pole */
      +4.67161242847606017e-02, +7.20521288369678992e-01, /* pole */
      +3.75652031178004331e-01, +6.52323004788938898e-01, /* pole */
      +1.42067047454823101e-02, +7.52590235344956504e-01, /* pole */
      +4.13889554006368121e-01, +6.66837955547387340e-01, /* pole */
      -1.45723361510231159e-02, +7.88650175975675216e-01, /* pole */
      +4.50040563279352768e-01, +6.85040896442122071e-01, /* pole */
      -3.94037387066513839e-02, +8.28438856508527111e-01, /* pole */
      +4.84140159239587364e-01, +7.06652722282038903e-01, /* pole */
      -6.00217970248033278e-02, +8.71826191183197485e-01, /* pole */
      +5.16249193749692359e-01, +7.31566167884948770e-01, /* pole */
      -7.60393017795072818e-02, +9.18780276444198796e-01, /* pole */
      +5.46395864356877037e-01, +7.59835844386661741e-01, /* pole */
      -8.68832931115681023e-02, +9.69317232158167896e-01, /* pole */
      +5.74526750630850791e-01, +7.91665559102883498e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.29645076149898614e-12;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              21,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     277204.24914717983,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
      +2.83933216694441548e-01, +9.58844058467041460e-01,
    };
    static const double poles[] = {
      +2.06409551696236093e-01, +6.41346540112892050e-01, /* pole */
      +1.63287105300986030e-01, +6.52784727603283543e-01, /* pole */
      +2.50106006130429492e-01, +6.35910759997425856e-01, /* pole */
      +1.21762488727462523e-01, +6.70089765209590849e-01, /* pole */
      +2.93370509818869918e-01, +6.36270274215034237e-01, /* pole */
      +8.26995231900126138e-02, +6.92848504416854527e-01, /* pole */
      +3.35393782185517575e-01, +6.41942474670548768e-01, /* pole */
      +4.67161242847606017e-02, +7.20521288369678992e-01, /* pole */
      +3.75652031178004331e-01, +6.52323004788938898e-01, /* pole */
      +1.42067047454822928e-02, +7.52590235344956615e-01, /* pole */
      +4.13889554006368121e-01, +6.66837955547387451e-01, /* pole */
      -1.45723361510231159e-02, +7.88650175975675216e-01, /* pole */
      +4.50040563279352768e-01, +6.85040896442122071e-01, /* pole */
      -3.94037387066513839e-02, +8.28438856508527111e-01, /* pole */
      +4.84140159239587364e-01, +7.06652722282038903e-01, /* pole */
      -6.00217970248033278e-02, +8.71826191183197485e-01, /* pole */
      +5.16249193749692359e-01, +7.31566167884948770e-01, /* pole */
      -7.60393017795072818e-02, +9.18780276444198796e-01, /* pole */
      +5.46395864356877037e-01, +7.59835844386661741e-01, /* pole */
      -8.68832931115681023e-02, +9.69317232158167896e-01, /* pole */
      +5.74526750630850791e-01, +7.91665559102883498e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.08132471292118384e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              20,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58622797698044465e-01, +3.83030065997859367e-02, /* pole */
      -1.60546761298405172e-01, +1.15348184618294325e-01, /* pole */
      -1.64487838685476451e-01, +1.93730239872415527e-01, /* pole */
      -1.70641924604227169e-01, +2.74406758715435961e-01, /* pole */
      -1.79328550528779995e-01, +3.58441111761084730e-01, /* pole */
      -1.91026946849032236e-01, +4.47058247494609928e-01, /* pole */
      -2.06434428680267290e-01, +5.41716267862435319e-01, /* pole */
      -2.26559760326192378e-01, +6.44202022477557090e-01, /* pole */
      -2.52873980388613251e-01, +7.56762532502384344e-01, /* pole */
      -2.87559576486839885e-01, +8.82289161304635638e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.51306260906793977e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              20,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.58622797698044465e-01, +3.83030065997859367e-02, /* pole */
      -1.60546761298405227e-01, +1.15348184618294325e-01, /* pole */
      -1.64487838685476451e-01, +1.93730239872415527e-01, /* pole */
      -1.70641924604227252e-01, +2.74406758715435961e-01, /* pole */
      -1.79328550528779995e-01, +3.58441111761084730e-01, /* pole */
      -1.91026946849032236e-01, +4.47058247494609928e-01, /* pole */
      -2.06434428680267290e-01, +5.41716267862435319e-01, /* pole */
      -2.26559760326192378e-01, +6.44202022477557090e-01, /* pole */
      -2.52873980388613417e-01, +7.56762532502384344e-01, /* pole */
      -2.87559576486839885e-01, +8.82289161304635638e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.54142743143970424e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              20,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     114443.44619817499,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.63213403468305063e-01, +2.40115156636743460e-01, /* pole */
      -4.81997863507479807e-01, +2.67401383021213412e-01, /* pole */
      -6.34404660598343106e-01, +2.39326314039766119e-01, /* pole */
      -4.13703873745191297e-01, +3.21408587587956873e-01, /* pole */
      -6.89940069358260466e-01, +2.48194272081830231e-01, /* pole */
      -3.64083665006227297e-01, +3.85747497145306206e-01, /* pole */
      -7.35011699106939642e-01, +2.59106236557471392e-01, /* pole */
      -3.28196414073414044e-01, +4.53268657416570520e-01, /* pole */
      -7.73630804879003153e-01, +2.70106370797352058e-01, /* pole */
      -3.02433776537361365e-01, +5.22498324413403936e-01, /* pole */
      -8.08187857991479519e-01, +2.80777283801156130e-01, /* pole */
      -2.84994808075282990e-01, +5.93524003666329847e-01, /* pole */
      -8.40184761736136876e-01, +2.91113381073964672e-01, /* pole */
      -2.75207126847806682e-01, +6.66866348601362846e-01, /* pole */
      -8.70659771243820213e-01, +3.01228781838420279e-01, /* pole */
      -2.73180860492469590e-01, +7.43174655576212873e-01, /* pole */
      -9.00409485800946707e-01, +3.11277974281014391e-01, /* pole */
      -2.79701423455231635e-01, +8.23113588138172370e-01, /* pole */
      -9.30115475558256577e-01, +3.21438224032055964e-01, /* pole */
      -2.96268086607717418e-01, +9.07269837444356630e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.07955818904662617e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              20,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     114443.44619817499,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
      -7.82534252313678791e-01, +6.22607536057725230e-01,
    };
    static const double poles[] = {
      -5.63213403468305063e-01, +2.40115156636743460e-01, /* pole */
      -4.81997863507479807e-01, +2.67401383021213412e-01, /* pole */
      -6.34404660598343106e-01, +2.39326314039766036e-01, /* pole */
      -4.13703873745191297e-01, +3.21408587587956762e-01, /* pole */
      -6.89940069358260466e-01, +2.48194272081830231e-01, /* pole */
      -3.64083665006227297e-01, +3.85747497145306206e-01, /* pole */
      -7.35011699106939642e-01, +2.59106236557471337e-01, /* pole */
      -3.28196414073414044e-01, +4.53268657416570520e-01, /* pole */
      -7.73630804879003153e-01, +2.70106370797352058e-01, /* pole */
      -3.02433776537361365e-01, +5.22498324413403936e-01, /* pole */
      -8.08187857991479519e-01, +2.80777283801156130e-01, /* pole */
      -2.84994808075282990e-01, +5.93524003666329847e-01, /* pole */
      -8.40184761736136876e-01, +2.91113381073964672e-01, /* pole */
      -2.75207126847806682e-01, +6.66866348601362846e-01, /* pole */
      -8.70659771243820213e-01, +3.01228781838420279e-01, /* pole */
      -2.73180860492469590e-01, +7.43174655576212873e-01, /* pole */
      -9.00409485800946818e-01, +3.11277974281014447e-01, /* pole */
      -2.79701423455231524e-01, +8.23113588138172481e-01, /* pole */
      -9.30115475558256577e-01, +3.21438224032055964e-01, /* pole */
      -2.96268086607717418e-01, +9.07269837444356630e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.06266387635144991e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14399.999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.60845213036122403e-01, +1.45308505601072363e-01, /* pole */
      -8.68155082767364417e-01, +2.68274674328105200e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.99940204219629347e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_BUTTERWORTH,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0,
      /* passband_edge = */      14399.999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.26542528005360455e-01, +0.00000000000000000e+00, /* pole */
      -7.60845213036122403e-01, +1.45308505601072363e-01, /* pole */
      -8.68155082767364528e-01, +2.68274674328105200e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.97957803700199797e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.05173410191827893,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.46300686792645185e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.23150343396322537e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 1.2115473324780848,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.51559591532746807e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.75779795766373459e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 1.2115473324780848,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     7582.8703111944797,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.31161353167099004e-01, +0.00000000000000000e+00, /* pole */
      -9.05978293123259837e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.40610328057684675e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.16123591697235959,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     7582.8703111944797,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.22497810243414529e-01, +6.91373209050995086e-01,
    };
    static const double poles[] = {
      -5.49434345080282771e-01, +4.68029948616943969e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.60465066178089177e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.0037181219695135541,
      /* passband_edge = */      3600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.58359486451806242e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.79179743225903065e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.005727081641956849,
      /* passband_edge = */      3600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +9.04764686792501327e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.52382343396250608e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.005727081641956849,
      /* passband_edge = */      3600,
      /* passband_edge2 = */     4911.4629877548641,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.45180016836385217e-01, +0.00000000000000000e+00, /* pole */
      -9.64909589310775528e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.07761151469635430e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.51806900775054121,
      /* passband_edge = */      3600,
      /* passband_edge2 = */     4911.4629877548641,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.49019227700874279e-01, +7.60772004002881697e-01,
    };
    static const double poles[] = {
      -5.75754506494157270e-01, +6.65384231885225552e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.87114713894911144e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.013832793046263506,
      /* passband_edge = */      7199.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.82266953581661939e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.91133476790830970e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.46671826808514083,
      /* passband_edge = */      7199.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.60336888749923256e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.19831555625038400e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.44747219195251581,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.69660613566158835e-01, +4.80993046822792647e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.81587456392766633e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.00023985602618272072,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +9.12735401526638457e-01, +8.07915919305060015e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.16245697428743400e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.00023985602618272072,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     33163.091124612241,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.77521455123663086e-01, +1.11251587821161027e-01, /* pole */
      +8.81689927948539287e-01, +1.07879790791420516e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.85163808798271856e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.0018226775243763925,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     33163.091124612241,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.74925445185994677e-02, +9.99846993737674272e-01,
      +1.74925445185994677e-02, +9.99846993737674272e-01,
    };
    static const double poles[] = {
      -7.69483178882657132e-02, +9.02180753266824120e-01, /* pole */
      +1.08486424878628751e-01, +8.99258260297497780e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.19459435962996863e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.14936084750789794,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.58671410251171086e-01, +3.85852651046568795e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.59447794472910820e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.00098143453247250814,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.77520026250943430e-01, +2.44289560148978180e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.18356533915192608e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.00098143453247250814,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     89184.276097659851,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.67986148121793288e-01, +3.18729819662696631e-02, /* pole */
      +6.24908149384080347e-01, +2.74615841294672369e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.57261247406877969e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.0047434683538103116,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     89184.276097659851,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -8.49517555077854203e-01, +5.27560350684682322e-01,
      -8.49517555077854203e-01, +5.27560350684682322e-01,
    };
    static const double poles[] = {
      -8.27714123805864177e-01, +4.00976314089857788e-01, /* pole */
      -7.00760933323422153e-01, +5.46659314672495178e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.16210855964843662e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 8000.0000,
      /* passband_ripple_db = */ 0.001088819760844719,
      /* passband_edge = */      3599.9999999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.60464230066272129e-01, +3.86152863020796655e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.61107297262236315e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 8000.0000,
      /* passband_ripple_db = */ 0.00365033950232636,
      /* passband_edge = */      3599.9999999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.41451379368271712e-01, +4.01662676638559868e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.84104840596419073e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.00096817850835462672,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.12581300635812412e-01, +0.00000000000000000e+00, /* pole */
      -1.90669904914917171e-01, +5.83045633612977010e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.44439262075616920e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.0044885231533463455,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.69026383904954858e-01, +0.00000000000000000e+00, /* pole */
      +7.28604661548320487e-01, +3.44675459279783880e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.09346154482313773e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.0044885231533463455,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     9805.5259831792591,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +1.26954194973186135e-01, +0.00000000000000000e+00, /* pole */
      +2.83960293813217343e-02, -0.00000000000000000e+00, /* pole */
      -5.08099363058185927e-01, +5.68236348330891761e-01, /* pole */
      +6.53171662719083224e-01, +4.73540443696016666e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.87295049446138562e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.034180937558078307,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     9805.5259831792591,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.54792199242376805e-01, +9.87947050733847920e-01,
      +1.54792199242376805e-01, +9.87947050733847920e-01,
      +1.54792199242376805e-01, +9.87947050733847920e-01,
    };
    static const double poles[] = {
      +1.08004630251667649e-01, +6.19527223847847308e-01, /* pole */
      -1.47176874991958279e-01, +8.58152794150314424e-01, /* pole */
      +3.97509093688983739e-01, +7.87410869008756675e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.92646558401250712e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.00033482592891757,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.07087271694715880e-01, +0.00000000000000000e+00, /* pole */
      -7.37951058067378041e-01, +3.21624852411054762e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.27550790742955722e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.24635339842651666,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.82327568537577234e-01, +0.00000000000000000e+00, /* pole */
      -1.28208809784495559e-01, +7.00144717854017551e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.12156278153830793e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.24635339842651666,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     20839.318894134252,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.46619561931856124e-01, +4.33378747822674493e-01, /* pole */
      -8.58560818275695303e-01, +3.44437559077384658e-01, /* pole */
      -2.09163746725227473e-01, +7.90665923843445917e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.29060859988502352e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 6.1231247034041099,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     20839.318894134252,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.35390628130952573e-01, +6.77643434305360226e-01,
      -7.35390628130952573e-01, +6.77643434305360226e-01,
      -7.35390628130952573e-01, +6.77643434305360226e-01,
    };
    static const double poles[] = {
      +4.72615163564300056e-01, +0.00000000000000000e+00, /* pole */
      -8.96441490081927528e-01, +0.00000000000000000e+00, /* pole */
      -9.05003474626288829e-01, +3.68239666150146749e-01, /* pole */
      -2.32119798661731519e-01, +9.13034368127308538e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.11787335757657430e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 8000.0000,
      /* passband_ripple_db = */ 0.23143914385038983,
      /* passband_edge = */      3599.9999999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.63682448948047288e-01, +0.00000000000000000e+00, /* pole */
      -8.84824932118887464e-01, +2.30333286145354865e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.49825516076879528e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 8000.0000,
      /* passband_ripple_db = */ 1.4332812511612141,
      /* passband_edge = */      3599.9999999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.72943741293730202e-01, +0.00000000000000000e+00, /* pole */
      -8.94918959993292962e-01, +2.73477925834197166e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.36318979016078479e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.0078793978405834169,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.64021639483602355e-01, +2.42211345538980438e-01, /* pole */
      +2.29067687877043263e-01, +7.12478553309121687e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.13088578977193754e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.069533459154117516,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.66384984689626736e-01, +2.92169729858961558e-01, /* pole */
      +5.86822915173454529e-01, +5.99041740004327639e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.01283678880706784e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.069533459154117516,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     20185.973502626894,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.55104386073958400e-01, +3.31180003799457334e-01, /* pole */
      +2.13574948570696227e-01, +4.35181653035849991e-01, /* pole */
      -8.33719044106340346e-01, +3.85634423841292928e-01, /* pole */
      +5.79912688409715882e-01, +6.36952161950445572e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.36447648275324046e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.32535744790931165,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     20185.973502626894,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -3.33015213951112110e-01, +9.42921453397415843e-01,
      -3.33015213951112110e-01, +9.42921453397415843e-01,
      -3.33015213951112110e-01, +9.42921453397415843e-01,
      -3.33015213951112110e-01, +9.42921453397415843e-01,
    };
    static const double poles[] = {
      -7.43383560534753518e-01, +2.09518233257180692e-01, /* pole */
      +5.15132335431765731e-01, +3.26218431798567354e-01, /* pole */
      -8.08939308147049951e-01, +4.62252567015162763e-01, /* pole */
      +4.96077102852154006e-01, +7.37214916767273132e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.58090119290151482e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 4.3797552393304562,
      /* passband_edge = */      600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.72469230782601968e-01, +5.99921198158546387e-01, /* pole */
      -2.36137025904722520e-01, +9.02668156300108215e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.66552593606517668e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.0005909665906612757,
      /* passband_edge = */      600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +1.15000275725207685e-01, +2.25853013368535055e-01, /* pole */
      +2.39927964756801393e-01, +6.78377918252159429e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.61575723543012195e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.0005909665906612757,
      /* passband_edge = */      600,
      /* passband_edge2 = */     959.17118354018783,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.27489021726538243e-01, +3.60460399254562344e-02, /* pole */
      +5.56484523502803266e-02, +2.44019010484499477e-01, /* pole */
      -9.75012236729861148e-01, +6.67757552656724485e-02, /* pole */
      +1.92331827993265703e-01, +7.01162839895966172e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.27220936309724197e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.47693900159246366,
      /* passband_edge = */      600,
      /* passband_edge2 = */     959.17118354018783,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -9.10840395712936624e-01, +4.12758735264925736e-01,
      -9.10840395712936624e-01, +4.12758735264925736e-01,
      -9.10840395712936624e-01, +4.12758735264925736e-01,
      -9.10840395712936624e-01, +4.12758735264925736e-01,
    };
    static const double poles[] = {
      -9.41822455618129273e-01, +5.81599079733947505e-02, /* pole */
      +5.74944641425531797e-02, +4.30570634700234323e-01, /* pole */
      -9.72877042395749903e-01, +1.28244722716804815e-01, /* pole */
      -2.97506146297648155e-01, +8.21090519073434333e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.65711241787356883e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.11506836438778993,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.98181283965764843e-01, +1.78964112752457161e-01, /* pole */
      -9.06723355330434355e-01, +2.50192951167059086e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.65090476214504345e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.28893329567067494,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.47356318836098299e-01, +1.17759330201650816e-01, /* pole */
      -8.88429707001079039e-01, +3.03519623619723677e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.34969117118398224e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.35119247884824151,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.61311912588494200e-01, +0.00000000000000000e+00, /* pole */
      +5.94572170611928463e-01, +4.41543921595164368e-01, /* pole */
      +5.15987523296237538e-01, +7.44356653751196196e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.99817868579073214e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 4.8909588781482993,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.94193254993096942e-01, +0.00000000000000000e+00, /* pole */
      +1.40901935858094352e-01, +8.28692130377555736e-01, /* pole */
      +5.41594245487170944e-01, +7.99885819242473817e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.60587638782091041e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 4.8909588781482993,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     1222.4581763330093,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.13482904264165477e-02, +8.97787958809702080e-01, /* pole */
      -4.57558088143490527e-01, +8.15530165528407514e-01, /* pole */
      +3.53066411387962409e-01, +8.61784270532295227e-01, /* pole */
      -6.42661867412574139e-01, +7.40893423593517730e-01, /* pole */
      +5.58829562842328431e-01, +8.03993264517861128e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.82220596927068672e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 4.2073451625122198,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     1222.4581763330093,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.77961507411856401e-02, +9.97699194118487065e-01,
      -6.77961507411856401e-02, +9.97699194118487065e-01,
      -6.77961507411856401e-02, +9.97699194118487065e-01,
      -6.77961507411856401e-02, +9.97699194118487065e-01,
      -6.77961507411856401e-02, +9.97699194118487065e-01,
    };
    static const double poles[] = {
      +8.25175679090290437e-01, +0.00000000000000000e+00, /* pole */
      -8.45662321261713368e-01, +0.00000000000000000e+00, /* pole */
      -7.56390729321781974e-01, +5.15569417303021416e-01, /* pole */
      +7.04836061616080367e-01, +5.69405311203574627e-01, /* pole */
      -6.68249561548066695e-01, +7.14626343673970377e-01, /* pole */
      +5.89442825574456264e-01, +7.78451770514949737e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.55412675774362614e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00090377669337527572,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.27424133578995130e-01, +0.00000000000000000e+00, /* pole */
      -3.22793062867974212e-01, +3.66127224887018932e-01, /* pole */
      -5.20558757904019065e-01, +6.42862354577418693e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.96930124552868180e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.0083895947132086816,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.42035855291097113e-01, +0.00000000000000000e+00, /* pole */
      -1.75127500151274235e-01, +4.38759230584719051e-01, /* pole */
      -6.74106368620801533e-02, +8.10397692341531362e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.15618742589223317e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.0083895947132086816,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     28544.021187211692,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.50371502003819790e-01, +3.32795760678791241e-01, /* pole */
      -8.02886758502943665e-01, +2.79938532833584219e-01, /* pole */
      -2.76219734256951510e-01, +5.86204797656646015e-01, /* pole */
      -9.18957853290158999e-01, +2.73946838867577203e-01, /* pole */
      -1.46520864826365643e-01, +8.53180516228979613e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.28081791395474892e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.56147640422110989,
      /* passband_edge = */      19200,
      /* passband_edge2 = */     28544.021187211692,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.78646630583852528e-01, +6.27462687879059011e-01,
      -7.78646630583852528e-01, +6.27462687879059011e-01,
      -7.78646630583852528e-01, +6.27462687879059011e-01,
      -7.78646630583852528e-01, +6.27462687879059011e-01,
      -7.78646630583852528e-01, +6.27462687879059011e-01,
    };
    static const double poles[] = {
      +2.01145053930279344e-01, +0.00000000000000000e+00, /* pole */
      -8.47115652882712067e-01, +0.00000000000000000e+00, /* pole */
      -8.87999789867928158e-01, +2.33043809502951144e-01, /* pole */
      -6.66908757775518946e-02, +7.16038256470721945e-01, /* pole */
      -9.21525914893502773e-01, +3.28143794391380694e-01, /* pole */
      -2.99767332508007578e-01, +8.90581108496701579e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.45959346871252227e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 8.8781759187556166e-05,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.13824346700241952e-01, +0.00000000000000000e+00, /* pole */
      -8.58403470881896502e-01, +1.08787738068898979e-01, /* pole */
      -9.41873178242395248e-01, +1.60024330816523258e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.45747761348582894e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.068992976376345758,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.31344385103579109e-01, +0.00000000000000000e+00, /* pole */
      -8.42811045606601916e-01, +1.84691348228750968e-01, /* pole */
      -8.90394910078153301e-01, +3.20278488542032724e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.55241032433013804e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              16,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.00095698181618734674,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +7.23838279992088052e-01, +7.78750425653373102e-02, /* pole */
      +7.05600904185642919e-01, +2.29394197703153224e-01, /* pole */
      +6.73094126527428838e-01, +3.69230517212930431e-01, /* pole */
      +6.33063294216693828e-01, +4.92474621266969281e-01, /* pole */
      +5.93341414488435914e-01, +5.97181134658179236e-01, /* pole */
      +5.61466109286629100e-01, +6.83749167030353044e-01, /* pole */
      +5.43999879218816096e-01, +7.53757600995588417e-01, /* pole */
      +5.46475327014464995e-01, +8.08714099419902355e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.68820307718988487e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              16,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.015472740420340135,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.70572431205514974e-01, +1.88551768650568469e-01, /* pole */
      -2.07690518939157825e-01, +4.97819383550119343e-01, /* pole */
      +1.58313156747070839e-02, +6.74309593649824901e-01, /* pole */
      +2.16636686703065406e-01, +7.47887531490197266e-01, /* pole */
      +3.68757501815824229e-01, +7.68992852302247898e-01, /* pole */
      +4.75518383280508883e-01, +7.71782608543097992e-01, /* pole */
      +5.46671352291325685e-01, +7.74376447758927888e-01, /* pole */
      +5.90229297877308712e-01, +7.86239167900960489e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.12338567615789375e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              16,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.015472740420340135,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     892.70766424004603,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.17070334918689184e-01, +8.91089039221347523e-01, /* pole */
      +2.86786730870189244e-01, +8.73004174627704188e-01, /* pole */
      +1.47167878876323599e-01, +9.07639492097234157e-01, /* pole */
      +3.53181197088423660e-01, +8.54291386921661178e-01, /* pole */
      +8.03921895152754362e-02, +9.22265060272534853e-01, /* pole */
      +4.13685337738195658e-01, +8.36206052380356613e-01, /* pole */
      +1.98195756525835799e-02, +9.35142375840724016e-01, /* pole */
      +4.66518463369919090e-01, +8.20140871786296466e-01, /* pole */
      -3.19964682649062568e-02, +9.46923949863591852e-01, /* pole */
      +5.10682020811694781e-01, +8.07442281593068079e-01, /* pole */
      -7.31157859612154293e-02, +9.58539589059131947e-01, /* pole */
      +5.45778659495181784e-01, +7.99294616889979714e-01, /* pole */
      -1.02133498055077696e-01, +9.70956672638633056e-01, /* pole */
      +5.71754143995040542e-01, +7.96673017290863261e-01, /* pole */
      -1.18014196632520940e-01, +9.84951581875347926e-01, /* pole */
      +5.88644453660362998e-01, +8.00339256078112182e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.31666019810090996e-11;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              16,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0049092748105542304,
      /* passband_edge = */      499.94999999999999,
      /* passband_edge2 = */     892.70766424004603,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
      +2.73808971578719029e-01, +9.61784095877554090e-01,
    };
    static const double poles[] = {
      -3.58647527191201410e-01, +2.14592915526775063e-01, /* pole */
      +5.84683419011849059e-01, +1.64121517260180344e-01, /* pole */
      -3.46907679854118023e-01, +5.33771108419052731e-01, /* pole */
      +6.20874543515304911e-01, +4.04598140425220687e-01, /* pole */
      -2.88532397466027823e-01, +7.25339210236404797e-01, /* pole */
      +6.23767769612797673e-01, +5.57418590513928724e-01, /* pole */
      -2.23832739271290132e-01, +8.35411768054695747e-01, /* pole */
      +6.10217593360053434e-01, +6.55645786879170633e-01, /* pole */
      -1.71302331135836150e-01, +8.99493247086285419e-01, /* pole */
      +5.94401534808330245e-01, +7.19357662125671093e-01, /* pole */
      -1.34126221382764144e-01, +9.39018904563589607e-01, /* pole */
      +5.82300336535000462e-01, +7.61342347241582229e-01, /* pole */
      -1.11087483209694998e-01, +9.65690450211567963e-01, /* pole */
      +5.76090481198717974e-01, +7.89354068260050390e-01, /* pole */
      -1.00718734099379481e-01, +9.85963526506475452e-01, /* pole */
      +5.76748870191594043e-01, +8.07978942343971496e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.02719834522609174e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.27335791727942765,
      /* passband_edge = */      26400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.38366899724885806e-01, +0.00000000000000000e+00, /* pole */
      +5.31439755915441570e-01, +4.20719878900149724e-01, /* pole */
      +2.93249875721939890e-01, +7.00809851876404433e-01, /* pole */
      +5.34490185343177576e-02, +8.35705897926133479e-01, /* pole */
      -1.28483466553953041e-01, +8.88418782042304622e-01, /* pole */
      -2.46353230354163122e-01, +9.10986508119243532e-01, /* pole */
      -3.08411740709224336e-01, +9.32220692611787527e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.50824617444961818e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.0034810040366813231,
      /* passband_edge = */      26400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.10011291196625338e-01, +0.00000000000000000e+00, /* pole */
      -5.84303182391887499e-01, +2.35084641184429927e-01, /* pole */
      -5.16548637984714731e-01, +4.44434576964263695e-01, /* pole */
      -4.28935651888819880e-01, +6.14314649646353517e-01, /* pole */
      -3.45159378212913870e-01, +7.45286573305309341e-01, /* pole */
      -2.83600210332157043e-01, +8.46465429673166070e-01, /* pole */
      -2.57216583926741571e-01, +9.28712940906617490e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.63847891820893790e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.0034810040366813231,
      /* passband_edge = */      26400,
      /* passband_edge2 = */     38478.948529022891,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.47301216344722929e-01, +5.61260314125279436e-01, /* pole */
      -7.23217639366281584e-01, +5.00707551607440626e-01, /* pole */
      -5.63278386519324048e-01, +6.30670104603999082e-01, /* pole */
      -7.86040880939715025e-01, +4.52029585045933435e-01, /* pole */
      -4.78783878686630027e-01, +7.03015031852322725e-01, /* pole */
      -8.34837051973465805e-01, +4.15507410160829149e-01, /* pole */
      -4.01657362499290238e-01, +7.72218987477153451e-01, /* pole */
      -8.71502596768647497e-01, +3.90366767318969909e-01, /* pole */
      -3.38847744789580185e-01, +8.34744891685334389e-01, /* pole */
      -8.98821994377368316e-01, +3.75840994877505941e-01, /* pole */
      -2.96276610444291044e-01, +8.89831060245579453e-01, /* pole */
      -9.19340530184294291e-01, +3.71587990121767908e-01, /* pole */
      -2.78965632903038496e-01, +9.38046379535918851e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.73350924033648912e-08;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              13,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.0079286655815737055,
      /* passband_edge = */      26400,
      /* passband_edge2 = */     38478.948529022891,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
      -7.46593696604227808e-01, +6.65280280927395440e-01,
    };
    static const double poles[] = {
      +2.50403147969359419e-01, +0.00000000000000000e+00, /* pole */
      -8.39965742930254389e-01, +0.00000000000000000e+00, /* pole */
      -8.51576608685315151e-01, +1.29225828362882295e-01, /* pole */
      +1.81519539679034919e-01, +4.02886669036257861e-01, /* pole */
      -8.70424603444029943e-01, +2.27395934187235360e-01, /* pole */
      +3.13911323948271107e-02, +6.64651213099175187e-01, /* pole */
      -8.84381553785744234e-01, +2.95770890504970407e-01, /* pole */
      -1.12697347441580442e-01, +7.97362406457709394e-01, /* pole */
      -8.94207388684252868e-01, +3.41931187389086289e-01, /* pole */
      -2.17611117858442449e-01, +8.61573085859290488e-01, /* pole */
      -9.02978673989274672e-01, +3.71606792294791544e-01, /* pole */
      -2.84700277473360408e-01, +8.98042354868005210e-01, /* pole */
      -9.13269495494290706e-01, +3.88453323693877972e-01, /* pole */
      -3.21734759008186599e-01, +9.27665312494931560e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.66741070629973090e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.00040419400196434863,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.92723942538932796e-01, +0.00000000000000000e+00, /* pole */
      -2.80611394191395414e-01, +2.57815876075578987e-01, /* pole */
      -4.57403127037863677e-01, +3.91816250107044295e-01, /* pole */
      -6.14556727176642115e-01, +4.22791958956006575e-01, /* pole */
      -7.25264682526067794e-01, +4.09137740882777934e-01, /* pole */
      -7.98976227651853277e-01, +3.83313408617523166e-01, /* pole */
      -8.48371541401509610e-01, +3.57769954237957488e-01, /* pole */
      -8.82386464114307323e-01, +3.36418951153183599e-01, /* pole */
      -9.06599460077165631e-01, +3.20133195423695227e-01, /* pole */
      -9.24440463050983263e-01, +3.08851185662417926e-01, /* pole */
      -9.38062917945826968e-01, +3.02337042347312013e-01, /* pole */
      -9.48872957054492816e-01, +3.00457218836380380e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.50756046094664548e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 5.8001490219974308,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.92226112629633894e-01, +0.00000000000000000e+00, /* pole */
      -9.91378568985270436e-01, +4.27950179685256532e-02, /* pole */
      -9.88906536605592246e-01, +8.46967287419610748e-02, /* pole */
      -9.85015227309082086e-01, +1.24846694297360533e-01, /* pole */
      -9.80025626575691700e-01, +1.62452077720298876e-01, /* pole */
      -9.74345918860652249e-01, +1.96808940388485759e-01, /* pole */
      -9.68436736410013266e-01, +2.27316253238400823e-01, /* pole */
      -9.62774091025311529e-01, +2.53480447333152281e-01, /* pole */
      -9.57813468973378490e-01, +2.74911849034764844e-01, /* pole */
      -9.53957738058674232e-01, +2.91315428630109508e-01, /* pole */
      -9.51530531003720981e-01, +3.02478818276789574e-01, /* pole */
      -9.50755897809928863e-01, +3.08260551895286228e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.10486448244258902e-17;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.074775049505962726,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.88759583420980603e-01, +5.98846644417246124e-02, /* pole */
      +8.75620743243100907e-01, +1.77552833800462773e-01, /* pole */
      +8.50757146037944056e-01, +2.89202835866594565e-01, /* pole */
      +8.16726889354927987e-01, +3.91692672699594180e-01, /* pole */
      +7.76787454429867852e-01, +4.82938819328071089e-01, /* pole */
      +7.34416500654546001e-01, +5.61953913399249383e-01, /* pole */
      +6.92917746738463181e-01, +6.28683212193526852e-01, /* pole */
      +6.55172362455445190e-01, +6.83728260845811175e-01, /* pole */
      +6.23536159325584705e-01, +7.28044488854888705e-01, /* pole */
      +5.99845534500059396e-01, +7.62670722430602077e-01, /* pole */
      +5.85483562997976437e-01, +7.88515302634659188e-01, /* pole */
      +5.81462712296498929e-01, +8.06199206477807961e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.99155719745799062e-15;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.079932634934918381,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.21141134237520154e-01, +1.71636255473997940e-01, /* pole */
      -4.94416220392309980e-01, +4.73411891545754471e-01, /* pole */
      -2.96856338390324381e-01, +6.81077712401952695e-01, /* pole */
      -9.00461020843706800e-02, +7.94950407179322194e-01, /* pole */
      +9.12070402725175394e-02, +8.42663019757290188e-01, /* pole */
      +2.36734517178512571e-01, +8.52572104158715427e-01, /* pole */
      +3.48601207711648930e-01, +8.44460706437587194e-01, /* pole */
      +4.32587165059536538e-01, +8.30073850052726048e-01, /* pole */
      +4.94463251305116258e-01, +8.15883001841851030e-01, /* pole */
      +5.38835480224152619e-01, +8.05364494132431608e-01, /* pole */
      +5.68991582554556508e-01, +8.00435455768045201e-01, /* pole */
      +5.87031364696312230e-01, +8.02279618672345096e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.75140850869868934e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.079932634934918381,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     19180.164953297935,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.54594806788149552e-01, +8.58777726139836384e-01, /* pole */
      -1.38226553658943180e-01, +8.81970702596589162e-01, /* pole */
      -3.62986035850824562e-01, +8.27301442716444457e-01, /* pole */
      -1.99652647529329173e-02, +8.94861516758869824e-01, /* pole */
      -4.59005810863815311e-01, +7.90717049629172886e-01, /* pole */
      +9.38466346512234206e-02, +8.97224315846965492e-01, /* pole */
      -5.40593291297477130e-01, +7.52431204773062912e-01, /* pole */
      +1.98080601734202194e-01, +8.90593107195632339e-01, /* pole */
      -6.07747735729588734e-01, +7.15345267253396422e-01, /* pole */
      +2.89610403763656432e-01, +8.77623883199024513e-01, /* pole */
      -6.61772053182567932e-01, +6.81529201217238811e-01, /* pole */
      +3.67269561616997309e-01, +8.61279413055284992e-01, /* pole */
      -7.04521580447782791e-01, +6.52244288669933114e-01, /* pole */
      +4.31311078987029528e-01, +8.44226670391229228e-01, /* pole */
      -7.37891269393351701e-01, +6.28139692530420346e-01, /* pole */
      +4.82770089616942955e-01, +8.28568458195221513e-01, /* pole */
      -7.63549780043238369e-01, +6.09480072868145761e-01, /* pole */
      +5.22956411306598756e-01, +8.15829236674474600e-01, /* pole */
      -7.82840733821739798e-01, +5.96334907714508478e-01, /* pole */
      +5.53129319867359803e-01, +8.07066036753589566e-01, /* pole */
      -7.96770840010747494e-01, +5.88713674122575026e-01, /* pole */
      +5.74318638987470376e-01, +8.03010055769998732e-01, /* pole */
      -8.06030795172398173e-01, +5.86655248171581767e-01, /* pole */
      +5.87235911606164396e-01, +8.04191240506399008e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.37922922444923009e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              24,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 4.1378526778703533,
      /* passband_edge = */      7200,
      /* passband_edge2 = */     19180.164953297935,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
      -2.19133931312011171e-01, +9.75694788418869696e-01,
    };
    static const double poles[] = {
      -9.75420109148811609e-01, +4.99893644828207848e-02, /* pole */
      +9.60815011324024804e-01, +7.69657263549599208e-02, /* pole */
      -9.66576533884920153e-01, +1.47328833440259238e-01, /* pole */
      +9.39070312183432843e-01, +2.25385270462798243e-01, /* pole */
      -9.50413490554814833e-01, +2.37377221641004110e-01, /* pole */
      +9.00061536845497834e-01, +3.58954836023958113e-01, /* pole */
      -9.29402885169334114e-01, +3.17028927853698628e-01, /* pole */
      +8.50715563183882306e-01, +4.72325158474453721e-01, /* pole */
      -9.06199642003565176e-01, +3.84931500722878461e-01, /* pole */
      +7.97916092088189255e-01, +5.64293939304885139e-01, /* pole */
      -8.83082219247110167e-01, +4.41118912896165671e-01, /* pole */
      +7.46970916582508915e-01, +6.36494298807140257e-01, /* pole */
      -8.61727558539565952e-01, +4.86444392573228634e-01, /* pole */
      +7.01269225416265019e-01, +6.91839948169274521e-01, /* pole */
      -8.43234201736026145e-01, +5.22089718931630586e-01, /* pole */
      +6.62633358337582301e-01, +7.33422747113759543e-01, /* pole */
      -8.28256212916650414e-01, +5.49244362744656378e-01, /* pole */
      +6.31856067071355665e-01, +7.63949432178958832e-01, /* pole */
      -8.17152703939519554e-01, +5.68934600107967592e-01, /* pole */
      +6.09162914229491759e-01, +7.85545003117328333e-01, /* pole */
      -8.10110636901012593e-01, +5.81949579011702522e-01, /* pole */
      +5.94533191644021408e-01, +7.99739259909464373e-01, /* pole */
      -8.07231123588797894e-01, +5.88817827696946128e-01, /* pole */
      +5.87897452776804497e-01, +8.07517357785408008e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.48689315686099660e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.68379616759012751,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.18021138090477939e-01, +1.50121322432858861e-01, /* pole */
      +7.27940751225721216e-01, +4.26226091401220308e-01, /* pole */
      +5.76469707097838402e-01, +6.42559395519806009e-01, /* pole */
      +4.01613268036385807e-01, +7.89444818359308575e-01, /* pole */
      +2.32670278783411399e-01, +8.77031067076433524e-01, /* pole */
      +8.51006935215149057e-02, +9.22578677491361798e-01, /* pole */
      -3.60915308063578938e-02, +9.42066421722546288e-01, /* pole */
      -1.31455422442619624e-01, +9.47370840756307286e-01, /* pole */
      -2.03649891936069499e-01, +9.46415800970112908e-01, /* pole */
      -2.55623280269368425e-01, +9.44199922841866379e-01, /* pole */
      -2.89813531550621339e-01, +9.43789259439147155e-01, /* pole */
      -3.07848866553331491e-01, +9.47034193867078877e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.71896090867014906e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.32221074494890833,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.82458279236671639e-01, +8.46553773216127126e-02, /* pole */
      -8.54664184724892473e-01, +2.49031597522989534e-01, /* pole */
      -8.03159042944643908e-01, +3.99692705749844168e-01, /* pole */
      -7.34870800882086117e-01, +5.30652579021194226e-01, /* pole */
      -6.57755627297769707e-01, +6.39334226690633112e-01, /* pole */
      -5.79207124783145400e-01, +7.26131228805922402e-01, /* pole */
      -5.05144790632131024e-01, +7.93404000226988315e-01, /* pole */
      -4.39787153743252945e-01, +8.44427527159547164e-01, /* pole */
      -3.85869441878388619e-01, +8.82585222629678090e-01, /* pole */
      -3.45044285790767513e-01, +9.10873149676745597e-01, /* pole */
      -3.18289788711159316e-01, +9.31650441011993569e-01, /* pole */
      -3.06241545035394913e-01, +9.46536930563254719e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.21631335901405301e-12;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              24,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.32221074494890833,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     439414507.4592582,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.47535592236064494e-01, +7.24049573905713006e-01, /* pole */
      -6.12126312632573999e-01, +7.52738216102609381e-01, /* pole */
      -6.80349098911130157e-01, +6.95710647504179436e-01, /* pole */
      -5.74789169550944057e-01, +7.81020428861044280e-01, /* pole */
      -7.10117638319174627e-01, +6.68444252801314143e-01, /* pole */
      -5.36389504995111399e-01, +8.08174450357150631e-01, /* pole */
      -7.36602855375417032e-01, +6.42885651021251281e-01, /* pole */
      -4.97940878155898492e-01, +8.33579830801549293e-01, /* pole */
      -7.59746067885082255e-01, +6.19550383144575112e-01, /* pole */
      -4.60536602716297039e-01, +8.56768309017050034e-01, /* pole */
      -7.79623741040103457e-01, +5.98825407468297155e-01, /* pole */
      -4.25275395405010914e-01, +8.77447872659933181e-01, /* pole */
      -7.96399758476854513e-01, +5.80979017873099912e-01, /* pole */
      -3.93194581855145331e-01, +8.95497095898287743e-01, /* pole */
      -8.10282059660824783e-01, +5.66182380250232375e-01, /* pole */
      -3.65220280767077632e-01, +9.10934448893765625e-01, /* pole */
      -8.21487620395464191e-01, +5.54535715296530451e-01, /* pole */
      -3.42138003347880026e-01, +9.23871824357410332e-01, /* pole */
      -8.30216627232619131e-01, +5.46093915759110926e-01, /* pole */
      -3.24581564706432790e-01, +9.34462325263944127e-01, /* pole */
      -8.36634706904314740e-01, +5.40888468214326679e-01, /* pole */
      -3.13034608027426553e-01, +9.42850475714890113e-01, /* pole */
      -8.40861211724557411e-01, +5.38944203114357978e-01, /* pole */
      -3.07837702238496935e-01, +9.49130164091088635e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -4.61783284442735292e-09;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              24,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.0031179593022504118,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     439414507.4592582,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
      -6.48844243246996855e-01, +7.60921249542442446e-01,
    };
    static const double poles[] = {
      -8.32104050231617043e-01, +6.12425129111070429e-02, /* pole */
      +3.83949290623905926e-01, +1.66269145406552243e-01, /* pole */
      -8.38787501500529387e-01, +1.73897052719051759e-01, /* pole */
      +3.03688409043790408e-01, +4.56740322402270305e-01, /* pole */
      -8.44846043321770490e-01, +2.66149549568800492e-01, /* pole */
      +1.82065010433994423e-01, +6.58638389843972938e-01, /* pole */
      -8.46964167944655677e-01, +3.38251598852171098e-01, /* pole */
      +5.97692691695529826e-02, +7.80036894285367266e-01, /* pole */
      -8.45810706686291658e-01, +3.93711121968188527e-01, /* pole */
      -4.35386793998943783e-02, +8.47935175091285798e-01, /* pole */
      -8.42964330900885073e-01, +4.36144965119737549e-01, /* pole */
      -1.24341800079757064e-01, +8.85023110635107635e-01, /* pole */
      -8.39710913615988597e-01, +4.68543290001825319e-01, /* pole */
      -1.85478880796417750e-01, +9.05521129912235878e-01, /* pole */
      -8.36887841837524071e-01, +4.93188237877075275e-01, /* pole */
      -2.30969602442207989e-01, +9.17475858673535982e-01, /* pole */
      -8.35004378009929549e-01, +5.11751810323009759e-01, /* pole */
      -2.64276302198916901e-01, +9.25334560859254363e-01, /* pole */
      -8.34378539354806215e-01, +5.25426186815392327e-01, /* pole */
      -2.87970376554512186e-01, +9.31605667009019922e-01, /* pole */
      -8.35237441611707987e-01, +5.35036898805552719e-01, /* pole */
      -3.03815923045473613e-01, +9.37791039991237141e-01, /* pole */
      -8.37784674564192033e-01, +5.41126484418245624e-01, /* pole */
      -3.12919427678684803e-01, +9.44896058766610203e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.72175301118364472e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.066552789089654743,
      /* passband_edge = */      86399.999999999985,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.62934266001875061e-01, +2.37073098745238020e-01, /* pole */
      -6.67536542541468991e-01, +3.82942436174825818e-01, /* pole */
      -8.30300067509699646e-01, +3.48561864551527656e-01, /* pole */
      -9.03399662780415502e-01, +3.12308051690519506e-01, /* pole */
      -9.41374268930944669e-01, +2.97124433754889228e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.70474286362426819e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 3.8898501842607462,
      /* passband_edge = */      86399.999999999985,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.75395046696671986e-01, +4.85094360422668736e-02, /* pole */
      -9.68831413599241076e-01, +1.40473073085750494e-01, /* pole */
      -9.58884735367345531e-01, +2.18162668164218615e-01, /* pole */
      -9.50523383462359761e-01, +2.74554052107174928e-01, /* pole */
      -9.48478113771607623e-01, +3.05109312816770673e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.40385544702971656e-11;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.013577997438945137,
      /* passband_edge = */      3007.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.52600076055495260e-01, +0.00000000000000000e+00, /* pole */
      +8.46051348439395334e-01, +1.20160475630719507e-01, /* pole */
      +8.27161262685333298e-01, +2.36129945223129523e-01, /* pole */
      +7.98047282216709064e-01, +3.44293236249406887e-01, /* pole */
      +7.61792201571129923e-01, +4.42022266514416007e-01, /* pole */
      +7.21940285720988895e-01, +5.27832071209809128e-01, /* pole */
      +6.82032778549158780e-01, +6.01290963733894457e-01, /* pole */
      +6.45279203493612008e-01, +6.62765376518886784e-01, /* pole */
      +6.14390645496543431e-01, +7.13098568623673823e-01, /* pole */
      +5.91547682800858787e-01, +7.53300131544154983e-01, /* pole */
      +5.78451601164929663e-01, +7.84286105646631926e-01, /* pole */
      +5.76406633550661573e-01, +8.06677903417461928e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.81264930136523702e-14;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.0080033220095222669,
      /* passband_edge = */      3007.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.04535645224815088e-01, +0.00000000000000000e+00, /* pole */
      -4.47583725388155385e-01, +2.95769600327201454e-01, /* pole */
      -3.02109421399872791e-01, +5.30796678889050244e-01, /* pole */
      -1.20561165917263430e-01, +6.82325608495807812e-01, /* pole */
      +5.47088371838348186e-02, +7.62797782864478524e-01, /* pole */
      +2.03754983140815693e-01, +7.96525547338438389e-01, /* pole */
      +3.22457993542802523e-01, +8.04569340948277367e-01, /* pole */
      +4.13713340423127962e-01, +8.01061304579721134e-01, /* pole */
      +4.82238859527656660e-01, +7.94481012622039073e-01, /* pole */
      +5.32403860806640150e-01, +7.89719305079774503e-01, /* pole */
      +5.67564577197541942e-01, +7.89659457250312280e-01, /* pole */
      +5.89968908445874129e-01, +7.96186130981601292e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.03939027946469259e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.0080033220095222669,
      /* passband_edge = */      3007.5,
      /* passband_edge2 = */     5111.5629833236198,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.94080640513474378e-01, +8.97175422786431920e-01, /* pole */
      +2.50875516929065623e-01, +9.09990458816966274e-01, /* pole */
      +3.36409869650761417e-01, +8.83738347012969605e-01, /* pole */
      +2.07754376135636853e-01, +9.21913283471266221e-01, /* pole */
      +3.76992647488547217e-01, +8.70044663048338829e-01, /* pole */
      +1.65712246984403511e-01, +9.32790107444315897e-01, /* pole */
      +4.15090271161828284e-01, +8.56524236673593342e-01, /* pole */
      +1.25723065878654144e-01, +9.42590460466924629e-01, /* pole */
      +4.50120112273072581e-01, +8.43636976454887222e-01, /* pole */
      +8.86909680821935664e-02, +9.51398516437596875e-01, /* pole */
      +4.81659420683418382e-01, +8.31840809240033785e-01, /* pole */
      +5.54160653251301594e-02, +9.59390713683437646e-01, /* pole */
      +5.09431095767899689e-01, +8.21566242090093768e-01, /* pole */
      +2.65775048378805792e-02, +9.66803976379615904e-01, /* pole */
      +5.33276752869308646e-01, +8.13199346757342290e-01, /* pole */
      +2.73250350794795481e-03, +9.73899638595540407e-01, /* pole */
      +5.53123298916830319e-01, +8.07072787921970458e-01, /* pole */
      -1.56728743733930177e-02, +9.80927470142051505e-01, /* pole */
      +5.68948457974594701e-01, +8.03462977748915064e-01, /* pole */
      -2.82868131871177458e-02, +9.88092662221314866e-01, /* pole */
      +5.80749093273811412e-01, +8.02590689117455680e-01, /* pole */
      -3.48344396325200034e-02, +9.95527037488408428e-01, /* pole */
      +5.88514548714548114e-01, +8.04622340237874489e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -2.25356084528303474e-15;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              23,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 1.9412094482035729,
      /* passband_edge = */      3007.5,
      /* passband_edge2 = */     5111.5629833236198,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
      +3.10964922162945911e-01, +9.50421389271197459e-01,
    };
    static const double poles[] = {
      +9.07866209980463745e-01, +0.00000000000000000e+00, /* pole */
      -8.31703198003988509e-01, +0.00000000000000000e+00, /* pole */
      -7.50933603399396410e-01, +4.12830947591901920e-01, /* pole */
      +8.86106184678681297e-01, +2.42458721826955048e-01, /* pole */
      -5.81973293101236133e-01, +6.89168560205202785e-01, /* pole */
      +8.34519452432103459e-01, +4.32122387484167536e-01, /* pole */
      -4.21151000899915917e-01, +8.36742021351697285e-01, /* pole */
      +7.76454906032361225e-01, +5.61212306979008035e-01, /* pole */
      -2.98398672027760048e-01, +9.10996432188810323e-01, /* pole */
      +7.25006889401363042e-01, +6.45695647433521258e-01, /* pole */
      -2.10264393602475980e-01, +9.49114203427183001e-01, /* pole */
      +6.83574309197307439e-01, +7.01496031498007877e-01, /* pole */
      -1.47668386076682034e-01, +9.69593644684395572e-01, /* pole */
      +6.51585833437985484e-01, +7.39190839825609292e-01, /* pole */
      -1.03263211075473788e-01, +9.81194504546915591e-01, /* pole */
      +6.27538037247043268e-01, +7.65151960124223818e-01, /* pole */
      -7.20675638506601823e-02, +9.88169811045754742e-01, /* pole */
      +6.10018477457845654e-01, +7.83168399483658018e-01, /* pole */
      -5.08777330816594861e-02, +9.92680480117534181e-01, /* pole */
      +5.97948785049013387e-01, +7.95514397599891843e-01, /* pole */
      -3.77048490690773744e-02, +9.95877892242161167e-01, /* pole */
      +5.90593859809988686e-01, +8.03559319092550961e-01, /* pole */
      -3.14055072491484580e-02, +9.98401056110688301e-01, /* pole */
      +5.87522376483909370e-01, +8.08102394281879466e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.94636192353107521e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.3549556085434083,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +4.39960090145727289e-01, +2.99017276558380285e-01, /* pole */
      +1.33132866572227254e-01, +6.96484534666762634e-01, /* pole */
      -1.51434865976549954e-01, +8.41876237208190381e-01, /* pole */
      -3.05226040608068461e-01, +9.06022297884588657e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.59915644082888833e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.58011622814583297,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.08256078549384216e-01, +2.14842167298947923e-01, /* pole */
      -5.62240244959772650e-01, +5.71029334134267152e-01, /* pole */
      -3.91093636920891718e-01, +7.91041354342307157e-01, /* pole */
      -2.93869279610534440e-01, +9.15111317344778685e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.30609881641306664e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.58011622814583297,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     19730.131562816317,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.59605651480228206e-01, +6.57269718979247797e-01, /* pole */
      -5.51839405597913557e-01, +7.38944884845466943e-01, /* pole */
      -7.45484008506358098e-01, +5.88750518898400577e-01, /* pole */
      -4.40094418528983566e-01, +8.18861166763796722e-01, /* pole */
      -8.04551601568366248e-01, +5.43199387990109361e-01, /* pole */
      -3.49588347606346717e-01, +8.85356124438739567e-01, /* pole */
      -8.40156479252246791e-01, +5.24539705847020254e-01, /* pole */
      -3.02560119315438081e-01, +9.35195741834045413e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.35625550936256041e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              8,
      /* sampling_frequency = */ 48000.0000,
      /* passband_ripple_db = */ 0.12778585712353605,
      /* passband_edge = */      14400,
      /* passband_edge2 = */     19730.131562816317,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
      -6.54947573597491672e-01, +7.55674318631219810e-01,
    };
    static const double poles[] = {
      -7.46484241190057962e-01, +1.85878734017138808e-01, /* pole */
      +8.75053978196655285e-02, +3.84532979880905013e-01, /* pole */
      -8.05213059934909658e-01, +3.88306037815072169e-01, /* pole */
      -9.62814208784907866e-02, +7.69780882825847668e-01, /* pole */
      -8.21460775760260442e-01, +4.84214327807004297e-01, /* pole */
      -2.44054320943721981e-01, +8.80580653239776079e-01, /* pole */
      -8.32961810511958389e-01, +5.28838653284492644e-01, /* pole */
      -3.13237042654802289e-01, +9.24946562675043049e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.80896054221074010e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              29,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0042397706459906988,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +4.78904818430637147e-02, +0.00000000000000000e+00, /* pole */
      -7.11406634893814588e-02, +3.36581423414308301e-01, /* pole */
      -3.06430580135982067e-01, +5.03911039598543664e-01, /* pole */
      -5.11015523978622488e-01, +5.35155806151253799e-01, /* pole */
      -6.52552252429235202e-01, +5.10023594663388646e-01, /* pole */
      -7.45526276640089969e-01, +4.70535651085898843e-01, /* pole */
      -8.07143789864525507e-01, +4.32001063362106863e-01, /* pole */
      -8.49106554249124290e-01, +3.98836151789023319e-01, /* pole */
      -8.78569095000800604e-01, +3.71711834622481918e-01, /* pole */
      -8.99856420068564256e-01, +3.50173108822694312e-01, /* pole */
      -9.15630069566917282e-01, +3.33536046740546599e-01, /* pole */
      -9.27571939970304626e-01, +3.21176854130250156e-01, /* pole */
      -9.36774680692795125e-01, +3.12612536336661695e-01, /* pole */
      -9.43966543567378191e-01, +3.07514369289164702e-01, /* pole */
      -9.49643548099004864e-01, +3.05704581392228836e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.56061539096145601e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              29,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0045751718651556635,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.55832061414781609e-01, +0.00000000000000000e+00, /* pole */
      -9.55525599155442440e-01, +3.30830941947710053e-02, /* pole */
      -9.54629106965090179e-01, +6.57736119739843195e-02, /* pole */
      -9.53209999414342479e-01, +9.76869567651758586e-02, /* pole */
      -9.51376500506525113e-01, +1.28453905692075043e-01, /* pole */
      -9.49271769185478176e-01, +1.57726903975571653e-01, /* pole */
      -9.47066280592024134e-01, +1.85184879710659223e-01, /* pole */
      -9.44948970326263682e-01, +2.10536372104421610e-01, /* pole */
      -9.43117675992190208e-01, +2.33520954269521241e-01, /* pole */
      -9.41769381923265536e-01, +2.53909109903256158e-01, /* pole */
      -9.41090703167649201e-01, +2.71500872015517536e-01, /* pole */
      -9.41248952590615695e-01, +2.86123638937375357e-01, /* pole */
      -9.42384040326688543e-01, +2.97629643309654746e-01, /* pole */
      -9.44601375163651502e-01, +3.05893564827278530e-01, /* pole */
      -9.47965885053391522e-01, +3.10810751992676770e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -5.40909398153520041e-18;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              12,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.030506818423078827,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +7.55491025278187345e-01, +1.06470054933836336e-01, /* pole */
      +7.21046547137290883e-01, +3.08532258981191021e-01, /* pole */
      +6.65074397290859376e-01, +4.82463999932164156e-01, /* pole */
      +6.06892631356373169e-01, +6.20883391572272125e-01, /* pole */
      +5.65156603688432235e-01, +7.24876052195603693e-01, /* pole */
      +5.54659517699591431e-01, +7.98757384305070595e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.33234396414448601e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              12,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.00016308604542192325,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.39212549075213886e-03, +1.43846630761259642e-01, /* pole */
      +1.22284985184640371e-01, +3.86296161747628319e-01, /* pole */
      +2.81517869359536599e-01, +5.39304083401775380e-01, /* pole */
      +4.27944856559913434e-01, +6.23154145773153201e-01, /* pole */
      +5.41825207586402735e-01, +6.75128324194126606e-01, /* pole */
      +6.24542470280217255e-01, +7.24220697510019495e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.50073438298753067e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              12,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.00016308604542192325,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     34959.131734651703,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.47536527108318555e-01, +5.56210214437622641e-01, /* pole */
      -2.86161110587665884e-02, +5.82836079222466275e-01, /* pole */
      -4.35324573693513406e-01, +5.49122319563099870e-01, /* pole */
      +1.72225359354803931e-01, +6.25196855831285481e-01, /* pole */
      -5.75700684624767201e-01, +5.44166309715872654e-01, /* pole */
      +3.33510702122234326e-01, +6.60716703675307437e-01, /* pole */
      -6.77275238555108960e-01, +5.39318492384241899e-01, /* pole */
      +4.57217910858823628e-01, +6.86985155234333233e-01, /* pole */
      -7.50704478348800719e-01, +5.39873024393926815e-01, /* pole */
      +5.48991974042953745e-01, +7.12271833953291078e-01, /* pole */
      -8.04674899443651226e-01, +5.51411633995916417e-01, /* pole */
      +6.14441031287044526e-01, +7.46257834851086321e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.81267487468011344e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              12,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.096365018414297177,
      /* passband_edge = */      13200,
      /* passband_edge2 = */     34959.131734651703,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
      -2.07436589268975125e-01, +9.78248466102786485e-01,
    };
    static const double poles[] = {
      -8.35154714437804868e-01, +9.37181139524953521e-02, /* pole */
      +7.56168652882400205e-01, +1.31136818751571738e-01, /* pole */
      -8.24646079076988170e-01, +2.65480843062612326e-01, /* pole */
      +7.20123198915034735e-01, +3.68107458276061450e-01, /* pole */
      -8.07571835134346605e-01, +4.01244417315507140e-01, /* pole */
      +6.66052499898591832e-01, +5.48579887888976825e-01, /* pole */
      -7.91168223977500751e-01, +4.98239010883789191e-01, /* pole */
      +6.15235255150937688e-01, +6.72134235024182791e-01, /* pole */
      -7.82213885722615410e-01, +5.62186861473086608e-01, /* pole */
      +5.81420129345405434e-01, +7.52012344876556438e-01, /* pole */
      -7.85455792782007167e-01, +5.99496540213373730e-01, /* pole */
      +5.71515652878805569e-01, +8.01092653046697745e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.05049924679677368e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.073957797260459146,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.95770654966835767e-01, +0.00000000000000000e+00, /* pole */
      +6.47786220697157522e-01, +2.89523084308277334e-01, /* pole */
      +5.21695330898354626e-01, +5.31457420364197852e-01, /* pole */
      +3.56976291137723001e-01, +7.03512881917188970e-01, /* pole */
      +1.89996461341612632e-01, +8.10342884788857631e-01, /* pole */
      +4.20814132906534161e-02, +8.69477993478400712e-01, /* pole */
      -7.87834700575128916e-02, +8.99131467650201577e-01, /* pole */
      -1.72088030479772280e-01, +9.13280457033583848e-01, /* pole */
      -2.40211689325567718e-01, +9.21317471660886178e-01, /* pole */
      -2.85928271901237541e-01, +9.29204137730151203e-01, /* pole */
      -3.11274592705937880e-01, +9.40683288700467135e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.09635078610176968e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.067201786410762965,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.24197201566106652e-01, +0.00000000000000000e+00, /* pole */
      -8.08007318641664529e-01, +1.80312778887785435e-01, /* pole */
      -7.62387995494436699e-01, +3.48620225715599752e-01, /* pole */
      -6.95099761618755085e-01, +4.95970669107326678e-01, /* pole */
      -6.16149518131263396e-01, +6.18032860539498019e-01, /* pole */
      -5.35209451710323414e-01, +7.14807954881947261e-01, /* pole */
      -4.60049875975994560e-01, +7.89201079881587164e-01, /* pole */
      -3.96131286489763645e-01, +8.45409038241354827e-01, /* pole */
      -3.46951907054020847e-01, +8.87694174620041299e-01, /* pole */
      -3.14682054035114456e-01, +9.19658884614476757e-01, /* pole */
      -3.00783830327372048e-01, +9.43889148787816934e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.93731414033303977e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.067201786410762965,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     448701099.23140687,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.45056346736265551e-01, +6.97097070170590971e-01, /* pole */
      -6.86006937766945435e-01, +6.61630786049997699e-01, /* pole */
      -6.00841540861455248e-01, +7.33109848282776921e-01, /* pole */
      -7.22844321197890327e-01, +6.27879807525411060e-01, /* pole */
      -5.54616024685127607e-01, +7.68439001911558894e-01, /* pole */
      -7.55135151212144251e-01, +5.96825546987846400e-01, /* pole */
      -5.07952773660521495e-01, +8.01952601449309577e-01, /* pole */
      -7.82805395817027794e-01, +5.69198392853217494e-01, /* pole */
      -4.62604232430270879e-01, +8.32760954530302411e-01, /* pole */
      -8.06042755048099635e-01, +5.45477075472227679e-01, /* pole */
      -4.20343814323137999e-01, +8.60300276103859063e-01, /* pole */
      -8.25189269598778696e-01, +5.25931664648059627e-01, /* pole */
      -3.82826797018248566e-01, +8.84336478904235945e-01, /* pole */
      -8.40647576984432710e-01, +5.10685925810453467e-01, /* pole */
      -3.51495460812231264e-01, +9.04899330584310047e-01, /* pole */
      -8.52811244406456836e-01, +4.99780250514111624e-01, /* pole */
      -3.27534238813687151e-01, +9.22175053661594690e-01, /* pole */
      -8.62018912011231486e-01, +4.93224839338706089e-01, /* pole */
      -3.11864416190060612e-01, +9.36387220974181478e-01, /* pole */
      -8.68526371470165426e-01, +4.91039511693076136e-01, /* pole */
      -3.05159126174499251e-01, +9.47687580504877669e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -1.16716279925472923e-10;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              21,
      /* sampling_frequency = */ 1073741824.0000,
      /* passband_ripple_db = */ 0.013777662315450682,
      /* passband_edge = */      322122547.19999999,
      /* passband_edge2 = */     448701099.23140687,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
      -6.78277705129577524e-01, +7.34805657792694134e-01,
    };
    static const double poles[] = {
      +4.49961472129833062e-01, +0.00000000000000000e+00, /* pole */
      -8.64419387493203351e-01, +0.00000000000000000e+00, /* pole */
      -8.67036451792914664e-01, +1.15346214023083316e-01, /* pole */
      +3.90961782325055018e-01, +3.54286928765947584e-01, /* pole */
      -8.71229216949890861e-01, +2.14733077877277079e-01, /* pole */
      +2.52743091640254491e-01, +6.14926693674601510e-01, /* pole */
      -8.72804598950634336e-01, +2.93408413118092237e-01, /* pole */
      +1.01784039261324455e-01, +7.68793372897009775e-01, /* pole */
      -8.71486989750583918e-01, +3.53482690175757064e-01, /* pole */
      -2.55902494100980100e-02, +8.49488499736057445e-01, /* pole */
      -8.68667739223731972e-01, +3.98657344277514591e-01, /* pole */
      -1.22313281610853697e-01, +8.89926900079806016e-01, /* pole */
      -8.65685208558470642e-01, +4.32317563453172959e-01, /* pole */
      -1.92670836493024328e-01, +9.10362526075185197e-01, /* pole */
      -8.63445598701745087e-01, +4.57093870081730413e-01, /* pole */
      -2.42687433519255397e-01, +9.21581572276536365e-01, /* pole */
      -8.62512415434973034e-01, +4.74885544887211464e-01, /* pole */
      -2.77251357115698760e-01, +9.29163887888963202e-01, /* pole */
      -8.63259139824024468e-01, +4.86995105825574703e-01, /* pole */
      -2.99733407509638061e-01, +9.36104553014988960e-01, /* pole */
      -8.65988667198967499e-01, +4.94256905525571610e-01, /* pole */
      -3.12207672223087651e-01, +9.44191005027204433e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.04765396796252902e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.09961477391495889,
      /* passband_edge = */      25199.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.03568136452420334e-01, +2.70505876170766546e-01, /* pole */
      -1.26995823141386549e-01, +5.88888084879205742e-01, /* pole */
      -4.34238575212271771e-01, +6.36605271805945283e-01, /* pole */
      -6.27249721322208131e-01, +5.87927131626312383e-01, /* pole */
      -7.41589769578666735e-01, +5.24865163867653473e-01, /* pole */
      -8.11333511068814239e-01, +4.69232463566627900e-01, /* pole */
      -8.55890692974700218e-01, +4.24440500961456968e-01, /* pole */
      -8.85640016255620610e-01, +3.89447505699214880e-01, /* pole */
      -9.06255074916214154e-01, +3.62520636436591936e-01, /* pole */
      -9.20967609528585140e-01, +3.42141407905158124e-01, /* pole */
      -9.31697074760298172e-01, +3.27156942818688456e-01, /* pole */
      -9.39625503482628188e-01, +3.16749629210618278e-01, /* pole */
      -9.45500561241973125e-01, +3.10379216876229158e-01, /* pole */
      -9.49800803125817050e-01, +3.07736532579444055e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.55358323689597198e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.058366214792113548,
      /* passband_edge = */      25199.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.68135076721430066e-01, +1.72902970094827718e-02, /* pole */
      -9.67323918004970862e-01, +5.16424645117856546e-02, /* pole */
      -9.65757498014656157e-01, +8.53147379310052328e-02, /* pole */
      -9.63544227630024452e-01, +1.17871376692752383e-01, /* pole */
      -9.60838552789464839e-01, +1.48901478701066126e-01, /* pole */
      -9.57831916723177135e-01, +1.78026225125742760e-01, /* pole */
      -9.54741864652710492e-01, +2.04903862542845538e-01, /* pole */
      -9.51800064199465501e-01, +2.29232303711791513e-01, /* pole */
      -9.49239993086869793e-01, +2.50749499966047729e-01, /* pole */
      -9.47284950206233711e-01, +2.69231969002156901e-01, /* pole */
      -9.46136906453360460e-01, +2.84492028593183788e-01, /* pole */
      -9.45966560638111131e-01, +2.96374379865548154e-01, /* pole */
      -9.46904832197865032e-01, +3.04752705222285036e-01, /* pole */
      -9.49035927025242310e-01, +3.09526905012936782e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.80794251931705654e-18;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 3.7307500170167462,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.90166160129224004e-01, +1.80026824903076726e-01, /* pole */
      +7.87490062660788581e-01, +4.88268943821684065e-01, /* pole */
      +6.61330697671671186e-01, +6.88206428019209060e-01, /* pole */
      +5.88121518903778728e-01, +7.89749632868563367e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.49643272027668149e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0.5898072876838919,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.16876264586758605e-01, +3.65607393619734378e-01, /* pole */
      +1.18897526896355327e-01, +7.28896506234140040e-01, /* pole */
      +4.27152836472602104e-01, +7.75617065537176908e-01, /* pole */
      +5.71077078502299718e-01, +7.80497887718691441e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.17239405223392334e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0.5898072876838919,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     199115.3617320435,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.91564113634697919e-01, +7.78976913189159181e-01, /* pole */
      +1.91763714763345909e-02, +8.21382925609788206e-01, /* pole */
      -5.33486591963941659e-01, +7.09970477796588173e-01, /* pole */
      +2.96398449228339356e-01, +8.17758605104395686e-01, /* pole */
      -6.78384788282201279e-01, +6.52454097617343853e-01, /* pole */
      +4.79539123497137953e-01, +7.94428570088490327e-01, /* pole */
      -7.53203291340421299e-01, +6.29880108910921166e-01, /* pole */
      +5.75952632223243888e-01, +7.89454675456606636e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.34511086241150468e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              8,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0.028681428027431895,
      /* passband_edge = */      76800,
      /* passband_edge2 = */     199115.3617320435,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
      -1.66674749773157077e-01, +9.86011930854822527e-01,
    };
    static const double poles[] = {
      -6.80541701020058953e-01, +1.51984816535286421e-01, /* pole */
      +5.74244706424723916e-01, +1.87837887215031890e-01, /* pole */
      -6.96358605879375370e-01, +3.96282109619298495e-01, /* pole */
      +5.62593447809304537e-01, +4.90354749851141747e-01, /* pole */
      -7.05043124574415869e-01, +5.51617729423021230e-01, /* pole */
      +5.39084442249063933e-01, +6.81235506405203362e-01, /* pole */
      -7.23767784279992554e-01, +6.41644553422998754e-01, /* pole */
      +5.36836382389837996e-01, +7.94955547817793318e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.70167025675556195e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.72620978724971907,
      /* passband_edge = */      16800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +7.11575723335930777e-01, +0.00000000000000000e+00, /* pole */
      +5.89501007635579333e-01, +4.53306128014382026e-01, /* pole */
      +3.24010748309320806e-01, +7.42430157579814232e-01, /* pole */
      +6.55547615433772352e-02, +8.70059946350082547e-01, /* pole */
      -1.24451950424442015e-01, +9.11882337009737864e-01, /* pole */
      -2.44144205101484352e-01, +9.24745148288227514e-01, /* pole */
      -3.04946520380772412e-01, +9.37727965241962780e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.81455229295657556e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.00067691823675662547,
      /* passband_edge = */      16800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.49216854621899397e-01, +0.00000000000000000e+00, /* pole */
      -5.26865920417511790e-01, +2.22983037226234720e-01, /* pole */
      -4.67532882785619497e-01, +4.24412119502734164e-01, /* pole */
      -3.89878282886146010e-01, +5.92323928092727270e-01, /* pole */
      -3.14653580688995127e-01, +7.26660419200025998e-01, /* pole */
      -2.58967971921047979e-01, +8.35123778992594712e-01, /* pole */
      -2.35902836272659483e-01, +9.27681765813500259e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.06710281570104483e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.00067691823675662547,
      /* passband_edge = */      16800,
      /* passband_edge2 = */     24587.754782414282,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.34485134120347682e-01, +5.31330495357885124e-01, /* pole */
      -7.14270984710903445e-01, +4.72711560324503122e-01, /* pole */
      -5.47438361050534894e-01, +6.01261806510399621e-01, /* pole */
      -7.80655867442663576e-01, +4.27339682627719208e-01, /* pole */
      -4.61663348660220074e-01, +6.76504391501675406e-01, /* pole */
      -8.32354251095213660e-01, +3.94187074505178259e-01, /* pole */
      -3.84756516452627262e-01, +7.50336554125380362e-01, /* pole */
      -8.71458607671993457e-01, +3.71801927529984977e-01, /* pole */
      -3.22900801858130693e-01, +8.18779368190760692e-01, /* pole */
      -9.01037465087519696e-01, +3.59269120731398939e-01, /* pole */
      -2.81490074469682883e-01, +8.80886965313646519e-01, /* pole */
      -9.23894607954587999e-01, +3.56318379191595092e-01, /* pole */
      -2.65515500625117251e-01, +9.37191692367322138e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.19886966449359875e-07;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              13,
      /* sampling_frequency = */ 56000.0000,
      /* passband_ripple_db = */ 0.02698000553257526,
      /* passband_edge = */      16800,
      /* passband_edge2 = */     24587.754782414282,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
      -7.53150163060954503e-01, +6.57848639035802596e-01,
    };
    static const double poles[] = {
      +3.44499155725496276e-01, +0.00000000000000000e+00, /* pole */
      -8.71524043228344869e-01, +0.00000000000000000e+00, /* pole */
      -8.78620304266135244e-01, +1.21378628548655734e-01, /* pole */
      +2.55302450495689837e-01, +4.27820193759699541e-01, /* pole */
      -8.90578295300235689e-01, +2.17078915038474873e-01, /* pole */
      +7.22111852192335174e-02, +6.95147833236748558e-01, /* pole */
      -8.99397346107369211e-01, +2.84922758173059787e-01, /* pole */
      -9.29810148795992553e-02, +8.21309994621450934e-01, /* pole */
      -9.05620555813770989e-01, +3.30731434669534530e-01, /* pole */
      -2.07856073516860251e-01, +8.77409985119683156e-01, /* pole */
      -9.11714829377609282e-01, +3.59870828510015561e-01, /* pole */
      -2.78793762041554760e-01, +9.07339658585971653e-01, /* pole */
      -9.19785166942405752e-01, +3.75926396407077978e-01, /* pole */
      -3.16399419262902826e-01, +9.31918029790703262e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.86430294412105572e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              29,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 1.5112534019755184,
      /* passband_edge = */      44.999999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.79562274082041640e-01, +0.00000000000000000e+00, /* pole */
      +2.24174116680512958e-01, +6.61394583583820195e-01, /* pole */
      -2.65962236463623658e-01, +7.91437661634492917e-01, /* pole */
      -5.55907481339698251e-01, +7.15582778294542199e-01, /* pole */
      -7.10663146951434177e-01, +6.18357046980368752e-01, /* pole */
      -7.97390074522901005e-01, +5.37506527469691964e-01, /* pole */
      -8.49398106400576780e-01, +4.75244959447293336e-01, /* pole */
      -8.82497079259686790e-01, +4.27949299582045584e-01, /* pole */
      -9.04580307421322338e-01, +3.92024585074041865e-01, /* pole */
      -9.19845148831735404e-01, +3.64761519076495955e-01, /* pole */
      -9.30655399853812182e-01, +3.44246883639618084e-01, /* pole */
      -9.38406387790324814e-01, +3.29158608652117157e-01, /* pole */
      -9.43950447530047643e-01, +3.18599634907967999e-01, /* pole */
      -9.47816978820224754e-01, +3.11984447312932678e-01, /* pole */
      -9.50330351623883574e-01, +3.08967708127667218e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.06436015041719532e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_CHEBYSHEV1,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              29,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.066471882415442135,
      /* passband_edge = */      44.999999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        0,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.70011861714280110e-01, +0.00000000000000000e+00, /* pole */
      -9.69619368546075777e-01, +3.33787863059430839e-02, /* pole */
      -9.68466624249542551e-01, +6.63448226656689899e-02, /* pole */
      -9.66626428387811654e-01, +9.84944694516152464e-02, /* pole */
      -9.64215540953478056e-01, +1.29441632983857580e-01, /* pole */
      -9.61388206789863586e-01, +1.58824936904538389e-01, /* pole */
      -9.58327800413959374e-01, +1.86313195544998200e-01, /* pole */
      -9.55237183198693107e-01, +2.11608956628427103e-01, /* pole */
      -9.52328392203289797e-01, +2.34450096481599313e-01, /* pole */
      -9.49812241371838950e-01, +2.54609652189173419e-01, /* pole */
      -9.47888328423166948e-01, +2.71894238927321674e-01, /* pole */
      -9.46735827275274278e-01, +2.86141513402627778e-01, /* pole */
      -9.46505329620325742e-01, +2.97217201634884298e-01, /* pole */
      -9.47311899990716522e-01, +3.05012214445124941e-01, /* pole */
      -9.49229439686247067e-01, +3.09440334509989923e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -5.00347283667759901e-19;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.74723646111778008,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -35.787130032540013,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.08514626268292125e-02, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.40425731313414648e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 5.042765693714796,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -35.787130032540013,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +1.39832804334950822e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.69916402167475411e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 5.042765693714796,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     21506.391711926201,
      /* stopband_edge = */      0,
      /* stopband_db = */        -99.742732473612904,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +3.63441612953472812e-02, +6.17383805797813379e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.08758169139172811e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 4.4714053259916637,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     21506.391711926201,
      /* stopband_edge = */      0,
      /* stopband_db = */        -99.742732473612904,
    };
    static const double zeros[] = {
      +5.25780698921051240e-02, +9.98616816685169217e-01,
    };
    static const double poles[] = {
      +2.78553710507313014e-02, +2.42498298903882525e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.29790673333825346e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.30574679506342189,
      /* passband_edge = */      6015,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -76.423838960789396,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.71936362248061636e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.35968181124030818e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.0076462174893844491,
      /* passband_edge = */      6015,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -76.423838960789396,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.90756189230491047e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.45378094615245468e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.0076462174893844491,
      /* passband_edge = */      6015,
      /* passband_edge2 = */     8514.9602854148234,
      /* stopband_edge = */      0,
      /* stopband_db = */        -95.86746384869447,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +8.40618642558083717e-01, +0.00000000000000000e+00, /* pole */
      -9.70114410495591994e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.07748129438420204e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.30769000624140219,
      /* passband_edge = */      6015,
      /* passband_edge2 = */     8514.9602854148234,
      /* stopband_edge = */      0,
      /* stopband_db = */        -95.86746384869447,
    };
    static const double zeros[] = {
      -7.01859849286564974e-01, +7.12315065093698796e-01,
    };
    static const double poles[] = {
      -6.31213311188626891e-01, +6.32658973284238479e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.99343810349388595e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 5.1088372061377836,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -29.39055351689322,
    };
    static const double zeros[] = {
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.16577231435094575e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.08288615717547287e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              1,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 3.2655911140285609,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -29.39055351689322,
    };
    static const double zeros[] = {
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.39756063475085712e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.30121968262457144e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 5.2635429559357076,
      /* passband_edge = */      150,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -114.34253300699622,
    };
    static const double zeros[] = {
      -9.99954599866138216e-01, +9.52880929346911452e-03,
    };
    static const double poles[] = {
      +6.12915812229067991e-01, +5.47256310987879968e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.12818667772852405e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0.35385923657084728,
      /* passband_edge = */      150,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -114.34253300699622,
    };
    static const double zeros[] = {
      +9.99999419644117538e-01, +1.07736318303078954e-03,
    };
    static const double poles[] = {
      +5.33335715851208203e-01, +3.80436208960791222e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.99053590756029486e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0.35385923657084728,
      /* passband_edge = */      150,
      /* passband_edge2 = */     301.45256944850803,
      /* stopband_edge = */      0,
      /* stopband_db = */        -150.24770888163124,
    };
    static const double zeros[] = {
      -9.99999953762712890e-01, +3.04096320303793915e-04,
      +9.99999976818053637e-01, +2.15322762883178364e-04,
    };
    static const double poles[] = {
      -2.82220127627911599e-01, +6.48258937835698656e-01, /* pole */
      +4.92146014054567149e-01, +5.66412017450274807e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.97986505755023329e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 1000.0000,
      /* passband_ripple_db = */ 0.00017826552492000144,
      /* passband_edge = */      150,
      /* passband_edge2 = */     301.45256944850803,
      /* stopband_edge = */      0,
      /* stopband_db = */        -150.24770888163124,
    };
    static const double zeros[] = {
      +1.70899240899481386e-01, +9.85288510772342807e-01,
      +1.70919391458211278e-01, +9.85285015426274891e-01,
    };
    static const double poles[] = {
      +1.24692970174531445e-01, +9.51346633482449078e-01, /* pole */
      +2.03109881274973175e-01, +9.38318877039213683e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.21107872490095403e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0021676133113630392,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -100.69076591074116,
    };
    static const double zeros[] = {
      -9.99999564290996235e-01, +9.33497626021338538e-04,
    };
    static const double poles[] = {
      -7.88501960893464471e-01, +1.77329044168631239e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.07344886769654435e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.012408304947242113,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -100.69076591074116,
    };
    static const double zeros[] = {
      +9.99996256542198259e-01, +2.73622031092467287e-03,
    };
    static const double poles[] = {
      +4.37928670404035714e-01, +3.58511478623459667e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.48259582218060815e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.012408304947242113,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1578.1072430969662,
      /* stopband_edge = */      0,
      /* stopband_db = */        -70.42720733144607,
    };
    static const double zeros[] = {
      -9.99999426551606563e-01, +1.07093251795816889e-03,
      +9.99844237464613039e-01, +1.76493855078997397e-02,
    };
    static const double poles[] = {
      -9.58211518224565739e-01, +4.35162876545328939e-02, /* pole */
      +3.81717953156233525e-01, +3.84555975502412439e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.91387442474707115e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              2,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.051783737406000209,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1578.1072430969662,
      /* stopband_edge = */      0,
      /* stopband_db = */        -70.42720733144607,
    };
    static const double zeros[] = {
      -8.87859586875704987e-01, +4.60114500959166173e-01,
      -8.83286380847130514e-01, +4.68833839872910496e-01,
    };
    static const double poles[] = {
      -8.59225111062280167e-01, +2.52291387183003812e-01, /* pole */
      -5.85310101596352106e-01, +5.08279000299557637e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.81987718723703673e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0.00069720082757420202,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -117.25762757383869,
    };
    static const double zeros[] = {
      -9.99999998256556610e-01, +5.90498671773852755e-05,
    };
    static const double poles[] = {
      -9.64577253385728750e-01, +3.46317609501480966e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.65113315659110960e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              2,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0.47037375363442335,
      /* passband_edge = */      471859.19999999995,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -117.25762757383869,
    };
    static const double zeros[] = {
      +9.99926051021688411e-01, +1.21611055489087311e-02,
    };
    static const double poles[] = {
      -7.56679347919204437e-01, +2.53210254607958152e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.92060085442872597e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.044171155977770826,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -146.8474484333307,
    };
    static const double zeros[] = {
      -9.99898588325894511e-01, +1.42412451661849369e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +2.10148525660657531e-01, +6.32905538002505041e-01, /* pole */
      +2.53549724445405911e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +9.55910492130912365e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.070198637994140237,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -146.8474484333307,
    };
    static const double zeros[] = {
      +9.99992015102735277e-01, +3.99621455515027698e-03,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +6.02398556497484416e-01, +4.82651049008962663e-01, /* pole */
      +3.47936933156492212e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.71886494568098613e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 0.070198637994140237,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     11536.169877781502,
      /* stopband_edge = */      0,
      /* stopband_db = */        -116.47939290386167,
    };
    static const double zeros[] = {
      -9.99880126060230134e-01, +1.54833300610110938e-02,
      +9.99858290922172932e-01, +1.68344312108067475e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.15185596396606749e-01, +5.43980854955277260e-01, /* pole */
      +5.75162938192934292e-01, +5.71770191967184371e-01, /* pole */
      -2.29765835752783198e-02, +3.13861285487113895e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.72258977699367433e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 32000.0000,
      /* passband_ripple_db = */ 1.0678660317965079,
      /* passband_edge = */      4800,
      /* passband_edge2 = */     11536.169877781502,
      /* stopband_edge = */      0,
      /* stopband_db = */        -116.47939290386167,
    };
    static const double zeros[] = {
      -5.74963076260403549e-02, +9.98345718981842434e-01,
      -2.61074937287370176e-02, +9.99659141293273046e-01,
      -4.18122176614265512e-02, +9.99125486840483634e-01,
    };
    static const double poles[] = {
      -5.69738931198376486e-01, +6.82906420018528082e-01, /* pole */
      +5.19902165103765190e-01, +7.14779121724078204e-01, /* pole */
      +4.68993568483045908e-01, +0.00000000000000000e+00, /* pole */
      -5.00981704665014127e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.91474742450869401e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.054662282879237641,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -80.049102348270864,
    };
    static const double zeros[] = {
      -9.97490769290465318e-01, +7.07966466742298850e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -4.39943991268102075e-01, +5.66568769149908946e-01, /* pole */
      -2.11689188723106048e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.63120037176028310e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.023770167449316343,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -80.049102348270864,
    };
    static const double zeros[] = {
      +9.93198252881266175e-01, +1.16435520669598225e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +1.01273187954772206e-01, +6.35731088922997234e-01, /* pole */
      -1.47912698848230387e-02, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.99809503068459654e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.023770167449316343,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     1690.3553469869601,
      /* stopband_edge = */      0,
      /* stopband_db = */        -48.820064240071666,
    };
    static const double zeros[] = {
      -9.94610583599105813e-01, +1.03681179548876373e-01,
      +8.46457105801500354e-01, +5.32456916602787489e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.37522000468911676e-01, +3.29148590668791818e-01, /* pole */
      -6.79346287283024985e-02, +7.49736599192838749e-01, /* pole */
      -4.47852302156860504e-01, +2.98427369112406804e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.46445416424388281e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              3,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.073052460763309957,
      /* passband_edge = */      1200,
      /* passband_edge2 = */     1690.3553469869601,
      /* stopband_edge = */      0,
      /* stopband_db = */        -48.820064240071666,
    };
    static const double zeros[] = {
      -7.40493868678074874e-01, +6.72063114930568140e-01,
      -6.42129839285464632e-01, +7.66595897131743920e-01,
      -6.94543552837801004e-01, +7.19450660720695079e-01,
    };
    static const double poles[] = {
      -7.84131823516553528e-01, +4.91222644346691217e-01, /* pole */
      -3.91614314012855624e-01, +7.85296854372188879e-01, /* pole */
      -5.03051171859370982e-01, +4.42176613680769715e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.53935498981032892e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 0.00049866364236658212,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -153.35836748106848,
    };
    static const double zeros[] = {
      -9.99999910257176827e-01, +4.23657454030652674e-04,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.44984929670292795e-01, +9.09126656589384491e-02, /* pole */
      -8.91826503982516838e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.96544067031849412e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              3,
      /* sampling_frequency = */ 20050.0000,
      /* passband_ripple_db = */ 5.1378770126247968,
      /* passband_edge = */      9022.5,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -153.35836748106848,
    };
    static const double zeros[] = {
      +9.96154691449310348e-01, +8.76118182868576428e-02,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.30702067303882719e-01, +2.66152590862127281e-01, /* pole */
      -9.35827713703459851e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.07913001442034607e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00071300549622268312,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -72.794577350891913,
    };
    static const double zeros[] = {
      -9.14214721906094985e-01, +4.05230110246217301e-01,
      -9.84665784685047729e-01, +1.74451404323895692e-01,
    };
    static const double poles[] = {
      +4.08806806100971698e-02, +7.12417090776397588e-01, /* pole */
      +7.99680637676787182e-02, +2.32824748477553412e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.45961198911967943e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00046290573615659982,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -72.794577350891913,
    };
    static const double zeros[] = {
      +9.94588777340880692e-01, +1.03890153467843152e-01,
      +9.99065442681649540e-01, +4.32231562859500471e-02,
    };
    static const double poles[] = {
      +7.50849185496401561e-01, +3.97332715013608562e-01, /* pole */
      +5.76870524982901700e-01, +1.56711093232658055e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.07462327569859983e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00046290573615659982,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     26093.406036460758,
      /* stopband_edge = */      0,
      /* stopband_db = */        -123.91975257652271,
    };
    static const double zeros[] = {
      -9.99863005144840611e-01, +1.65520676269841661e-02,
      +9.99600481653265427e-01, +2.82644136425946983e-02,
      -9.99976484116090414e-01, +6.85793079743677019e-03,
      +9.99931413126771584e-01, +1.17119188136511291e-02,
    };
    static const double poles[] = {
      -8.54509535033097345e-01, +2.88319041670575205e-01, /* pole */
      +7.20878955496820573e-01, +4.41200401146498500e-01, /* pole */
      -6.82134584070119177e-01, +1.42467751668471976e-01, /* pole */
      +5.03142831867227081e-01, +1.96187849455083052e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.83534535683670985e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.0065314399094185561,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     26093.406036460758,
      /* stopband_edge = */      0,
      /* stopband_db = */        -123.91975257652271,
    };
    static const double zeros[] = {
      -2.94257689335009565e-01, +9.55726117811594111e-01,
      -2.27927765811065941e-01, +9.73678044105122042e-01,
      -2.75094948810897466e-01, +9.61417063057822818e-01,
      -2.47601627143821529e-01, +9.68861927333163453e-01,
    };
    static const double poles[] = {
      -6.51967182301523507e-01, +5.98079330549697641e-01, /* pole */
      +3.23447856124625399e-01, +7.81902554510688130e-01, /* pole */
      -4.51693143172453182e-01, +3.61638455061524700e-01, /* pole */
      +1.69863307009073122e-01, +4.28262406534530071e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.59196042877524840e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.098201582276946378,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -22.823232945210346,
    };
    static const double zeros[] = {
      -5.74683088554612365e-01, +8.18376042983500085e-01,
      -8.84339516146694704e-01, +4.66844321140816476e-01,
    };
    static const double poles[] = {
      -3.57646971815950843e-01, +8.12634148478083906e-01, /* pole */
      -1.82624569108215506e-01, +3.91893818946383143e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.23728957494496039e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.0068356434879317755,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -22.823232945210346,
    };
    static const double zeros[] = {
      +2.64877049960176969e-01, +9.64282193346114758e-01,
      +7.92786570260453405e-01, +6.09499347017424697e-01,
    };
    static const double poles[] = {
      -1.22316968300921480e-02, +8.25593376436011717e-01, /* pole */
      +2.85992639870347246e-02, +3.37248860741832690e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.13922352132165206e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.0068356434879317755,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     88160.853525868355,
      /* stopband_edge = */      0,
      /* stopband_db = */        -82.393376483243202,
    };
    static const double zeros[] = {
      -9.99424232364215959e-01, +3.39293937935478476e-02,
      +9.36493309659875539e-01, +3.50685444468818219e-01,
      -9.99900197766263155e-01, +1.41277920068133053e-02,
      +9.88697827829788656e-01, +1.49921997200735874e-01,
    };
    static const double poles[] = {
      -9.37167137155740626e-01, +1.78909621762304766e-01, /* pole */
      -3.16763942314166463e-02, +7.79751574535491376e-01, /* pole */
      -7.99141021063520585e-01, +1.41610293912812685e-01, /* pole */
      -2.07883095951013674e-01, +3.47002966382073841e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.38237817002226726e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              4,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.64532478214727818,
      /* passband_edge = */      57600,
      /* passband_edge2 = */     88160.853525868355,
      /* stopband_edge = */      0,
      /* stopband_db = */        -82.393376483243202,
    };
    static const double zeros[] = {
      -8.78762277210262233e-01, +4.77259740762023710e-01,
      -7.60449805944924395e-01, +6.49396714372906803e-01,
      -8.51621670161637034e-01, +5.24156971632643409e-01,
      -8.02488208699892014e-01, +5.96667977100865055e-01,
    };
    static const double poles[] = {
      -9.40008188686467738e-01, +2.49167030069972845e-01, /* pole */
      -2.93185237523161890e-01, +8.53281203025758606e-01, /* pole */
      -8.87246281824432437e-01, +1.24975909858254242e-01, /* pole */
      +3.18485024940159955e-02, +4.84440150663384361e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.02384828173368780e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.26340758465038833,
      /* passband_edge = */      1799.9999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -56.79045749425277,
    };
    static const double zeros[] = {
      -9.93906760896646868e-01, +1.10224092846961857e-01,
      -9.98894580150157640e-01, +4.70065713133844543e-02,
    };
    static const double poles[] = {
      -9.10040037017278758e-01, +2.70629661566341162e-01, /* pole */
      -6.81323273054474932e-01, +2.17841789566162930e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.50915612395270737e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              4,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.00088462352877968981,
      /* passband_edge = */      1799.9999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -56.79045749425277,
    };
    static const double zeros[] = {
      -9.83128931807634859e-02, +9.95155553184740915e-01,
      +6.50594988146242281e-01, +7.59424888582795221e-01,
    };
    static const double poles[] = {
      -7.29516208517078324e-01, +4.29938447947685709e-01, /* pole */
      -6.05859917125925240e-01, +1.56580298466823975e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +7.79430231257714854e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 4.3354219383140054,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -148.55711483184814,
    };
    static const double zeros[] = {
      -9.26930678289253018e-01, +3.75232618046759558e-01,
      -9.71359154187688922e-01, +2.37616063379094078e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.91627666614841119e-01, +7.62244345875906393e-01, /* pole */
      +7.46692731141405330e-01, +5.00176417561829068e-01, /* pole */
      +8.65315303028509120e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.04177488946609396e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0.046591228755119986,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -148.55711483184814,
    };
    static const double zeros[] = {
      +9.98140847013731292e-01, +6.09495654021510042e-02,
      +9.99288670342373786e-01, +3.77114482003895538e-02,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.91675844413057983e-01, +6.52143046231730361e-01, /* pole */
      +3.16805974421673242e-01, +4.66743972131638485e-01, /* pole */
      +1.04307786912223213e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.99546589572024619e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 0.046591228755119986,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     292130.47222641815,
      /* stopband_edge = */      0,
      /* stopband_db = */        -31.653014023855174,
    };
    static const double zeros[] = {
      -3.09402406467448876e-01, +9.50931201965816042e-01,
      +6.71518068048383654e-01, +7.40988180934464280e-01,
      -4.89804927064846185e-01, +8.71832055744109935e-01,
      +7.73676453667357045e-01, +6.33580890684608655e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.04057830179688371e-01, +9.38760655361285945e-01, /* pole */
      +5.90925495757763097e-01, +7.66711484777355623e-01, /* pole */
      -1.15729654020736705e-01, +8.20715609992251460e-01, /* pole */
      +4.84044540913694421e-01, +7.04876578961535438e-01, /* pole */
      +1.77581417549420140e-01, +6.60744854608709686e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.87225716837758754e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 1048576.0000,
      /* passband_ripple_db = */ 8.9323059547242278e-06,
      /* passband_edge = */      157286.39999999999,
      /* passband_edge2 = */     292130.47222641815,
      /* stopband_edge = */      0,
      /* stopband_db = */        -31.653014023855174,
    };
    static const double zeros[] = {
      +8.74461240221784675e-02, +9.96169250375405713e-01,
      +3.85027086752656733e-01, +9.22905272748380301e-01,
      +1.45036638087100073e-01, +9.89426285082618762e-01,
      +3.34202547030277797e-01, +9.42501277218484645e-01,
      +2.41916938468028281e-01, +9.70296962214278080e-01,
    };
    static const double poles[] = {
      +2.84676489363162727e-02, +9.58494069841637986e-01, /* pole */
      +4.17513668713331287e-01, +8.67658084201651914e-01, /* pole */
      +8.52676814444018882e-02, +8.75065989105335729e-01, /* pole */
      +3.37319603673685020e-01, +8.20584461269428300e-01, /* pole */
      +2.06404707036147439e-01, +8.14743402617801649e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.05353895515744589e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 3.2959883806240562e-05,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -54.762551909421376,
    };
    static const double zeros[] = {
      -9.37395814649637504e-01, +3.48265827604923750e-01,
      -9.75144039179203559e-01, +2.21571890936706928e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -6.42544365554726493e-01, +5.59222306063803298e-01, /* pole */
      -4.96601348446684909e-01, +3.10057992529101989e-01, /* pole */
      -4.38566559293108282e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.30484808688906839e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0047029036415772232,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -54.762551909421376,
    };
    static const double zeros[] = {
      +5.42237584838384912e-01, +8.40225208850957639e-01,
      +7.86683091889755937e-01, +6.17357038458924423e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -5.71999074981463587e-02, +8.29608542511476976e-01, /* pole */
      -1.17245674920904722e-01, +4.64042176698362563e-01, /* pole */
      -1.57985285213979032e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.99156819267973664e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.0047029036415772232,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1395.1762108718299,
      /* stopband_edge = */      0,
      /* stopband_db = */        -110.55103589108472,
    };
    static const double zeros[] = {
      -9.95778001718974015e-01, +9.17941789688601073e-02,
      +8.89251508674469560e-01, +4.57418576710850122e-01,
      -9.98356345435476689e-01, +5.73114956070690240e-02,
      +9.55432059292685154e-01, +2.95211077156225798e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.55422799166280945e-01, +4.08974860447865407e-01, /* pole */
      -1.62251792131777789e-01, +8.70975079055208656e-01, /* pole */
      -7.20875576732542434e-01, +4.07871909226017737e-01, /* pole */
      -2.75790326235756778e-01, +6.50517365575452700e-01, /* pole */
      -5.00242650945227019e-01, +4.68515096598169034e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.50236300520706538e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 5.5440669007552756,
      /* passband_edge = */      999.89999999999998,
      /* passband_edge2 = */     1395.1762108718299,
      /* stopband_edge = */      0,
      /* stopband_db = */        -110.55103589108472,
    };
    static const double zeros[] = {
      -7.44596775197028604e-01, +6.67514525958938010e-01,
      -6.04485605618125588e-01, +7.96616063483839065e-01,
      -7.22027752920933685e-01, +6.91864093599275476e-01,
      -6.34572878467170587e-01, +7.72863029206268481e-01,
      -6.80718428813590015e-01, +7.32545166302772999e-01,
    };
    static const double poles[] = {
      -8.69114463527165038e-01, +4.77044534296546141e-01, /* pole */
      -2.86438280536391976e-01, +9.40375800603291290e-01, /* pole */
      -8.89330662902912183e-01, +3.64428068706677433e-01, /* pole */
      -3.72037021670068371e-02, +9.00112090484970206e-01, /* pole */
      +5.85999304579357450e-01, +0.00000000000000000e+00, /* pole */
      -9.05509512607259204e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.22826843277113185e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.10651189519771441,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -56.60374969977596,
    };
    static const double zeros[] = {
      -9.87339358863072780e-01, +1.58622162511599746e-01,
      -9.94700879180471786e-01, +1.02811288084511940e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.24683102993627992e-01, +2.75481156478816547e-01, /* pole */
      -7.86871319158933935e-01, +2.66132047703957930e-01, /* pole */
      -5.76424899711483252e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.13301613593452921e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 3333.0000,
      /* passband_ripple_db = */ 0.059406567321597707,
      /* passband_edge = */      1499.8499999999999,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -56.60374969977596,
    };
    static const double zeros[] = {
      -8.02725491305939398e-01, +5.96348711416096133e-01,
      -5.81122925198082174e-01, +8.13815793536365462e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.98127175781365583e-01, +3.22383426202698775e-01, /* pole */
      -8.37770589853596204e-01, +2.01815841578023264e-01, /* pole */
      -8.10706784378658507e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.19459510616865560e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.008708610903015862,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -39.390780449668753,
    };
    static const double zeros[] = {
      +5.63461091748294285e-01, +8.26142601543958799e-01,
      +5.45654316916320603e-01, +8.38010361768029344e-01,
      +4.82816106037734061e-01, +8.75721763889969473e-01,
      +2.58285052151847216e-01, +9.66068751091203382e-01,
      -5.81160939582568981e-01, +8.13788647195022974e-01,
    };
    static const double poles[] = {
      +5.75396820217329075e-01, +8.08521581937061917e-01, /* pole */
      +5.65347903648999561e-01, +7.85204496046232325e-01, /* pole */
      +5.43139431108136450e-01, +7.23050355488242369e-01, /* pole */
      +4.95659387399111528e-01, +5.61757567120848789e-01, /* pole */
      +4.36524053885948615e-01, +2.24855834256962622e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.67506623154043811e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.44184195063689458,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -39.390780449668753,
    };
    static const double zeros[] = {
      +5.93084858044660934e-01, +8.05139957496921355e-01,
      +5.99827258271500185e-01, +8.00129527160756027e-01,
      +6.27939325827987838e-01, +7.78262297094425737e-01,
      +7.27406527785322776e-01, +6.86206778846799481e-01,
      +9.43160135753757900e-01, +3.32338619972089522e-01,
    };
    static const double poles[] = {
      +5.87170608770336777e-01, +8.07462715602341397e-01, /* pole */
      +5.78432471834439133e-01, +8.05162408400618368e-01, /* pole */
      +5.43079965734758252e-01, +7.96941593031771478e-01, /* pole */
      +4.07626448219768567e-01, +7.49869375685443540e-01, /* pole */
      +4.06214833753470866e-02, +4.25149974397477814e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.05856337418330843e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.44184195063689458,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     5556.386619834997,
      /* stopband_edge = */      0,
      /* stopband_db = */        -103.05695260877108,
    };
    static const double zeros[] = {
      -6.75740242128845758e-01, +7.37139827419227678e-01,
      +6.87011183274038983e-01, +7.26646842734766651e-01,
      -7.00398603262739283e-01, +7.13751915267205272e-01,
      +7.10960281387798765e-01, +7.03232165283259714e-01,
      -7.58843795593651782e-01, +6.51272672456798207e-01,
      +7.67626700065867529e-01, +6.40897222139390044e-01,
      -8.62388474594766530e-01, +5.06247092718676073e-01,
      +8.67683979429120789e-01, +4.97116195513730208e-01,
      -9.78500279517959370e-01, +2.06245492031401995e-01,
      +9.79377099667492979e-01, +2.02040829158091961e-01,
    };
    static const double poles[] = {
      -5.70240088794843003e-01, +8.11228935054599254e-01, /* pole */
      +5.84095486398509944e-01, +8.01436585876258656e-01, /* pole */
      -5.34973798634090669e-01, +8.10883011353002447e-01, /* pole */
      +5.49241258268257426e-01, +8.01686032155018879e-01, /* pole */
      -4.60580946847160777e-01, +8.20585208193893356e-01, /* pole */
      +4.75809564685144792e-01, +8.12540869171819335e-01, /* pole */
      -3.25059372111351608e-01, +8.35659298932327266e-01, /* pole */
      +3.41705643673083148e-01, +8.29817745652542293e-01, /* pole */
      -1.15120429699647281e-01, +8.46283749591834833e-01, /* pole */
      +1.32993823897283814e-01, +8.44077518824497663e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.72689502316799435e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              10,
      /* sampling_frequency = */ 16000.0000,
      /* passband_ripple_db = */ 0.28684950310801405,
      /* passband_edge = */      2400,
      /* passband_edge2 = */     5556.386619834997,
      /* stopband_edge = */      0,
      /* stopband_db = */        -103.05695260877108,
    };
    static const double zeros[] = {
      -4.66392677768683983e-01, +8.84577792013656938e-01,
      +4.82693986510658923e-01, +8.75789081563847605e-01,
      -4.41123756170990700e-01, +8.97446283485310414e-01,
      +4.57911642499607086e-01, +8.88997709593962848e-01,
      -3.80867304626308911e-01, +9.24629707648791932e-01,
      +3.98710005173373683e-01, +9.17077058798576816e-01,
      -2.68086382622789665e-01, +9.63394878257211373e-01,
      +2.87502258785822473e-01, +9.57779959694840310e-01,
      -9.28147059541541697e-02, +9.95683398655739182e-01,
      +1.13630172870318333e-01, +9.93523116899381109e-01,
    };
    static const double poles[] = {
      -5.66049131196036059e-01, +8.12814061246772690e-01, /* pole */
      +5.79982128824057863e-01, +8.03072735108031566e-01, /* pole */
      -5.75454161362226935e-01, +7.76595087104681947e-01, /* pole */
      +5.88677607970373717e-01, +7.67141764486917510e-01, /* pole */
      -6.04745232421119994e-01, +6.99268797602550896e-01, /* pole */
      +6.16421792749374364e-01, +6.90342447402191306e-01, /* pole */
      -6.49771409802122779e-01, +5.36084398410782148e-01, /* pole */
      +6.58770151623149114e-01, +5.28754954441752800e-01, /* pole */
      -6.81440562750642798e-01, +2.18086203845841348e-01, /* pole */
      +6.87524962048246535e-01, +2.14967835860692569e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.23518368406856181e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0.32822127520222144,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -65.160186532147918,
    };
    static const double zeros[] = {
      -3.09302266562679173e-01, +9.50963778437007190e-01,
      -3.09456006796833438e-01, +9.50913760473239833e-01,
      -3.09920225473288458e-01, +9.50762564388494491e-01,
      -3.11167790147421297e-01, +9.50354989661637117e-01,
      -3.14466489672183602e-01, +9.49268574679081212e-01,
      -3.23147006866087327e-01, +9.46348779231784509e-01,
      -3.45819772655133417e-01, +9.38300956431758038e-01,
      -4.03829559069554145e-01, +9.14834240298038615e-01,
      -5.42867014292152450e-01, +8.39818673758522705e-01,
      -8.06301325960827486e-01, +5.91505005687873675e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.09015594950721784e-01, +9.51013712276191692e-01, /* pole */
      -3.08851493372584052e-01, +9.50936380794046854e-01, /* pole */
      -3.08381411489007662e-01, +9.50780944262217154e-01, /* pole */
      -3.07126472167629394e-01, +9.50387855186265784e-01, /* pole */
      -3.03811161531122864e-01, +9.49349284989242870e-01, /* pole */
      -2.95086457680545922e-01, +9.46558483438475151e-01, /* pole */
      -2.72287425453181742e-01, +9.38845083225154808e-01, /* pole */
      -2.13880260058456756e-01, +9.16205004929308342e-01, /* pole */
      -7.34529269068953106e-02, +8.42931729671534513e-01, /* pole */
      +1.94320291670812412e-01, +5.96188562693536039e-01, /* pole */
      +3.92647208961684258e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.72914795429066387e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 3.8891921683977566,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -65.160186532147918,
    };
    static const double zeros[] = {
      -3.08964209502584852e-01, +9.51073665520417388e-01,
      -3.08923588897479007e-01, +9.51086860503656317e-01,
      -3.08784251826313250e-01, +9.51132107450938680e-01,
      -3.08346863265473059e-01, +9.51273994133311596e-01,
      -3.06985152622762381e-01, +9.51714303806126094e-01,
      -3.02744111808527616e-01, +9.53071877019391334e-01,
      -2.89484845398741641e-01, +9.57182597148771075e-01,
      -2.47555455226454585e-01, +9.68873725821700060e-01,
      -1.11215248028256497e-01, +9.93796341614323619e-01,
      +3.34336074148001205e-01, +9.42453919045012078e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.09018689493836485e-01, +9.51051618660007536e-01, /* pole */
      -3.09053419526123641e-01, +9.51025423871811770e-01, /* pole */
      -3.09175084865412730e-01, +9.50943433868481014e-01, /* pole */
      -3.09557535186857358e-01, +9.50688348440950071e-01, /* pole */
      -3.10746405006359749e-01, +9.49894867955461075e-01, /* pole */
      -3.14429532673111800e-01, +9.47423256629264632e-01, /* pole */
      -3.25755787794975549e-01, +9.39691535369514774e-01, /* pole */
      -3.59790848625940130e-01, +9.15222324533002896e-01, /* pole */
      -4.54624989188997908e-01, +8.36027725719598935e-01, /* pole */
      -6.57916006496942485e-01, +5.83981700903494549e-01, /* pole */
      -8.18448808346923018e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.62199600624911733e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              21,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 3.8891921683977566,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     56948.122403901798,
      /* stopband_edge = */      0,
      /* stopband_db = */        -153.70837973585029,
    };
    static const double zeros[] = {
      -9.43042735598345727e-01, +3.32671608099020966e-01,
      -2.89989022443502609e-01, +9.57029971768001531e-01,
      -9.43373926643530147e-01, +3.31731268543028868e-01,
      -2.87237939522240970e-01, +9.57859262156511848e-01,
      -9.44125369472696985e-01, +3.29586539042545479e-01,
      -2.80919768009876059e-01, +9.59731256103122488e-01,
      -9.45495916183568985e-01, +3.25633954740892184e-01,
      -2.69116039469267043e-01, +9.63107759962703147e-01,
      -9.47836199354817999e-01, +3.18757806481055062e-01,
      -2.48080203027654767e-01, +9.68739496906034891e-01,
      -9.51705921772893260e-01, +3.07011137358903730e-01,
      -2.10633155444407444e-01, +9.77565176255543622e-01,
      -9.57907707238837003e-01, +2.87076338997895253e-01,
      -1.42547899571997205e-01, +9.89787904718789635e-01,
      -9.67341293119491574e-01, +2.53477459798519378e-01,
      -1.42445071480604580e-02, +9.99898541861177437e-01,
      -9.80195619672904028e-01, +1.98031682248198948e-01,
      +2.34659662638121019e-01, +9.72077590900316535e-01,
      -9.93664681136214867e-01, +1.12385503791478630e-01,
      +6.70904354861793673e-01, +7.41543893931762010e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.40545009841438184e-01, +3.39332415196608450e-01, /* pole */
      -3.09158215548931192e-01, +9.50673846182932381e-01, /* pole */
      -9.39964537524905386e-01, +3.40172661720166469e-01, /* pole */
      -3.11557914997112551e-01, +9.49126556150656997e-01, /* pole */
      -9.38854215977674156e-01, +3.42162245150927968e-01, /* pole */
      -3.17174767901398647e-01, +9.46210121805396431e-01, /* pole */
      -9.36896209900670018e-01, +3.45847320176913497e-01, /* pole */
      -3.27420759686110119e-01, +9.41105457069215778e-01, /* pole */
      -9.33522148607920044e-01, +3.52229267245870559e-01, /* pole */
      -3.44719759407939086e-01, +9.32380464224538574e-01, /* pole */
      -9.27733649398119087e-01, +3.63007120305658737e-01, /* pole */
      -3.72715035092844094e-01, +9.17623797631129334e-01, /* pole */
      -9.17781422621128717e-01, +3.80932716259118842e-01, /* pole */
      -4.16109746623507082e-01, +8.92968281952232656e-01, /* pole */
      -9.00634924426960026e-01, +4.10188808271140237e-01, /* pole */
      -4.79335981624930574e-01, +8.52878023870381030e-01, /* pole */
      -8.71335028626900820e-01, +4.56309037351122382e-01, /* pole */
      -5.63029916653985607e-01, +7.91531487394215749e-01, /* pole */
      -8.23146899464006543e-01, +5.24203392514613564e-01, /* pole */
      -6.59184828790039257e-01, +7.07912268696115121e-01, /* pole */
      -7.51052178397816572e-01, +6.12489286146035705e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.06886420201143830e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              21,
      /* sampling_frequency = */ 128000.0000,
      /* passband_ripple_db = */ 0.0035107933370497999,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     56948.122403901798,
      /* stopband_edge = */      0,
      /* stopband_db = */        -153.70837973585029,
    };
    static const double zeros[] = {
      -9.34692466952516732e-01, +3.55457440803000013e-01,
      -3.53213645425453526e-01, +9.35542687794235217e-01,
      -9.34095067964017312e-01, +3.57024374525462596e-01,
      -3.57326846244909069e-01, +9.33979402852475604e-01,
      -9.32777556338586722e-01, +3.60452535559142251e-01,
      -3.66224480799148044e-01, +9.30526533562259228e-01,
      -9.30465411997629332e-01, +3.66379744358338977e-01,
      -3.81285385459360804e-01, +9.24457384001613480e-01,
      -9.26665321420724997e-01, +3.75887459322899520e-01,
      -4.04612959486962853e-01, +9.14488027814033932e-01,
      -9.20540853717361940e-01, +3.90646306314689717e-01,
      -4.38885296982558848e-01, +8.98543096402465880e-01,
      -9.10701359061383608e-01, +4.13065412015759192e-01,
      -4.86747565598935228e-01, +8.73542676337859425e-01,
      -8.94888201907942493e-01, +4.46290383143048974e-01,
      -5.49359560833342031e-01, +8.35586065537834100e-01,
      -8.69669265891534260e-01, +4.93634852865637686e-01,
      -6.24097043151228270e-01, +7.81346837665510718e-01,
      -8.30683462347733825e-01, +5.56744991339824358e-01,
      -7.02931340677797500e-01, +7.11257710181699254e-01,
      -7.74591031481910774e-01, +6.32462436787979865e-01,
    };
    static const double poles[] = {
      -9.39267479390140769e-01, +3.40131520195214032e-01, /* pole */
      -3.11506416121510976e-01, +9.47180849189456997e-01, /* pole */
      -9.37591361619546482e-01, +3.38038565960892379e-01, /* pole */
      -3.05861696029096020e-01, +9.42287142995093419e-01, /* pole */
      -9.35868637733343500e-01, +3.34168019606698785e-01, /* pole */
      -2.95309589528659433e-01, +9.36908452426429061e-01, /* pole */
      -9.33742248677401854e-01, +3.27787813665851635e-01, /* pole */
      -2.77806818685256196e-01, +9.29609119605300349e-01, /* pole */
      -9.30749035294245797e-01, +3.17701624629776302e-01, /* pole */
      -2.49998074726894282e-01, +9.18042495234842049e-01, /* pole */
      -9.26205428611080461e-01, +3.02048985561003569e-01, /* pole */
      -2.06762410146244691e-01, +8.97739712526185563e-01, /* pole */
      -9.19056448770333101e-01, +2.78021555427889089e-01, /* pole */
      -1.41064240538572361e-01, +8.59551648110726596e-01, /* pole */
      -9.07776055136107818e-01, +2.41497983832437563e-01, /* pole */
      -4.58118425047106051e-02, +7.84680458991727225e-01, /* pole */
      -8.90855313699099205e-01, +1.86663374290866102e-01, /* pole */
      +7.73413208854881046e-02, +6.38312790994672863e-01, /* pole */
      -8.69987404931828356e-01, +1.06379450097118869e-01, /* pole */
      +1.97492268957726536e-01, +3.76102648224285185e-01, /* pole */
      +2.49919620196095760e-01, +0.00000000000000000e+00, /* pole */
      -8.58347101880949093e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.08028816362609584e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.00041274984877214872,
      /* passband_edge = */      1799.9999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -82.083424949659431,
    };
    static const double zeros[] = {
      -9.52849184354958911e-01, +3.03444281333640542e-01,
      -9.53227668777454018e-01, +3.02253224097114304e-01,
      -9.54131857746788214e-01, +2.99386703166093837e-01,
      -9.55896700287361178e-01, +2.93703078601050549e-01,
      -9.59108475760875989e-01, +2.83038745975969064e-01,
      -9.64608777988288701e-01, +2.63685239306147678e-01,
      -9.73182615437457099e-01, +2.30033904045274212e-01,
      -9.84479933007909036e-01, +1.75497183751599978e-01,
      -9.95319944722730554e-01, +9.66344019339939048e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.50723948705337829e-01, +3.07381218753893570e-01, /* pole */
      -9.48650986800431029e-01, +3.07325319915705941e-01, /* pole */
      -9.45184897121542966e-01, +3.07672881945057708e-01, /* pole */
      -9.38839172950785783e-01, +3.08489877214538355e-01, /* pole */
      -9.26857704521974912e-01, +3.09827279534426347e-01, /* pole */
      -9.03969435312882541e-01, +3.11168889528952441e-01, /* pole */
      -8.60259801403304158e-01, +3.09084692385741933e-01, /* pole */
      -7.80065578269862558e-01, +2.88511339866944549e-01, /* pole */
      -6.60228041769950758e-01, +2.02239016435437291e-01, /* pole */
      -5.85562995524304575e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.67426037578994336e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 4000.0000,
      /* passband_ripple_db = */ 0.0081859399712477954,
      /* passband_edge = */      1799.9999999999998,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -82.083424949659431,
    };
    static const double zeros[] = {
      -9.50087147062080195e-01, +3.11984635819517964e-01,
      -9.49824247982653858e-01, +3.12784107563325475e-01,
      -9.49155730059729974e-01, +3.14806925106137059e-01,
      -9.47707332689350590e-01, +3.19140739434558174e-01,
      -9.44611889900143664e-01, +3.28189545018239159e-01,
      -9.37753360788161072e-01, +3.47301935397010042e-01,
      -9.21119215340543973e-01, +3.89280607185126359e-01,
      -8.71882834346364044e-01, +4.89714532326896212e-01,
      -6.39087571774133445e-01, +7.69133977668287505e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.50521160676451538e-01, +3.09329653121965475e-01, /* pole */
      -9.49704367674390593e-01, +3.08490771651441398e-01, /* pole */
      -9.48440311886269649e-01, +3.06637456635524885e-01, /* pole */
      -9.46109344954646048e-01, +3.02826298410650785e-01, /* pole */
      -9.41648914879391508e-01, +2.95158681170578197e-01, /* pole */
      -9.33316406469824278e-01, +2.79989305763226504e-01, /* pole */
      -9.18866292201089951e-01, +2.50821107872587490e-01, /* pole */
      -8.97435466779980873e-01, +1.97915756170644080e-01, /* pole */
      -8.74431033362265020e-01, +1.12524908624651893e-01, /* pole */
      -8.63755363118185682e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.16222122523705184e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 4.1893306424295877,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -96.778397559234847,
    };
    static const double zeros[] = {
      +3.26178062763456955e-01, +9.45308347245425673e-01,
      +2.14983036248703185e-01, +9.76617783027366682e-01,
      -1.16589569876429516e-01, +9.93180181133327511e-01,
      -8.04974976634218020e-01, +5.93308761938284634e-01,
    };
    static const double poles[] = {
      +5.88667360448705046e-01, +7.96214857400255327e-01, /* pole */
      +6.43468864037023969e-01, +7.21101388472932037e-01, /* pole */
      +7.58545216441663617e-01, +5.43522105552744494e-01, /* pole */
      +8.75961896219087710e-01, +2.11740125622258357e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.00669828907275699e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.064300583211073958,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -96.778397559234847,
    };
    static const double zeros[] = {
      +8.43123052901891268e-01, +5.37720668809926527e-01,
      +8.78862491538056623e-01, +4.77075173287731746e-01,
      +9.39700513170067087e-01, +3.41998458399906335e-01,
      +9.91921853429858280e-01, +1.26850450090942535e-01,
    };
    static const double poles[] = {
      +5.85720209071346476e-01, +7.59142861039375338e-01, /* pole */
      +4.63606961226229797e-01, +7.17031051589551494e-01, /* pole */
      +2.29718496902289848e-01, +6.17135525681577701e-01, /* pole */
      -6.24437200905008372e-02, +2.80930621810203396e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.90327089208617461e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              8,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.064300583211073958,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     30043.395843491839,
      /* stopband_edge = */      0,
      /* stopband_db = */        -87.704070029354554,
    };
    static const double zeros[] = {
      -9.92098092916721463e-01, +1.25464632590242126e-01,
      +8.00259644707683182e-01, +5.99653650912201686e-01,
      -9.93863254598647106e-01, +1.10615691285571319e-01,
      +8.41468185960366100e-01, +5.40306664790071611e-01,
      -9.96919855497543028e-01, +7.84270470868184272e-02,
      +9.17280922920607145e-01, +3.98240766931160950e-01,
      -9.99584881098583344e-01, +2.88108569662724780e-02,
      +9.88453309244859746e-01, +1.51525758347832412e-01,
    };
    static const double poles[] = {
      -9.74529296411813184e-01, +1.83667118662681428e-01, /* pole */
      +5.86027347502323104e-01, +7.66465298367886128e-01, /* pole */
      -9.48576504172531698e-01, +1.94629052306778627e-01, /* pole */
      +4.80012950588424858e-01, +7.31646791501248894e-01, /* pole */
      -8.87090026614267946e-01, +2.20758088915553041e-01, /* pole */
      +2.67421804050675505e-01, +6.60758839188467229e-01, /* pole */
      -7.02913774442922801e-01, +2.36195512396036894e-01, /* pole */
      -9.13913242123817471e-02, +4.25395163347190519e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.40665747020740559e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              8,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00098198397573910742,
      /* passband_edge = */      9600,
      /* passband_edge2 = */     30043.395843491839,
      /* stopband_edge = */      0,
      /* stopband_db = */        -87.704070029354554,
    };
    static const double zeros[] = {
      -9.45102369083622107e-01, +3.26774405289834424e-01,
      +1.17666410930779675e-01, +9.93053178706291062e-01,
      -9.32903555268673523e-01, +3.60126306408000640e-01,
      +1.47368630782076220e-02, +9.99891406537037120e-01,
      -8.95471314041989097e-01, +4.45119226419072256e-01,
      -2.13375494092407997e-01, +9.76970264911282027e-01,
      -7.87947571867854135e-01, +6.15742335711580768e-01,
      -5.36758318146693059e-01, +8.43736041603257214e-01,
    };
    static const double poles[] = {
      -9.60789434617369764e-01, +2.11215844040818196e-01, /* pole */
      +4.70604920978962948e-01, +8.09250379327550062e-01, /* pole */
      -9.29482455133497254e-01, +1.86373137469345113e-01, /* pole */
      +4.45610457857286035e-01, +6.63749680128656760e-01, /* pole */
      -8.94419555945085043e-01, +1.36601117885819828e-01, /* pole */
      +4.36158243880291785e-01, +4.54408066270536204e-01, /* pole */
      -8.63919816253107986e-01, +5.26345844669240254e-02, /* pole */
      +4.28411943608104029e-01, +1.65549147413312531e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.50233687710491562e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.90032689131095645,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -107.52488334779183,
    };
    static const double zeros[] = {
      -3.09558141530718922e-01, +9.50880516685481014e-01,
      -3.09692091177345530e-01, +9.50836899085328202e-01,
      -3.10024433374160191e-01, +9.50728589404479241e-01,
      -3.10714968125035462e-01, +9.50503134441469499e-01,
      -3.12095317559033969e-01, +9.50050794830321510e-01,
      -3.14826721357720773e-01, +9.49149163998550871e-01,
      -3.20211033011938651e-01, +9.47346237833574301e-01,
      -3.30788319332084912e-01, +9.43704979214083917e-01,
      -3.51442768954705831e-01, +9.36209367689433747e-01,
      -3.91276067266413119e-01, +9.20273350252265754e-01,
      -4.65991070454469414e-01, +8.84789422550189442e-01,
      -5.97095492632105151e-01, +8.02170164415520803e-01,
      -7.90979255308583706e-01, +6.11842968147447430e-01,
      -9.70468994147722697e-01, +2.41225892884465082e-01,
    };
    static const double poles[] = {
      -3.09006467833717879e-01, +9.51028139463359179e-01, /* pole */
      -3.08856116562616967e-01, +9.50998076972088824e-01, /* pole */
      -3.08501718181277529e-01, +9.50980904863427345e-01, /* pole */
      -3.07772638776732166e-01, +9.50967978876177900e-01, /* pole */
      -3.06317854078532459e-01, +9.50951606590351495e-01, /* pole */
      -3.03436999382399264e-01, +9.50918174936191796e-01, /* pole */
      -2.97743498596068079e-01, +9.50829235303233711e-01, /* pole */
      -2.86499041537691346e-01, +9.50554800842910108e-01, /* pole */
      -2.64308998858007516e-01, +9.49623157056276201e-01, /* pole */
      -2.20622642485286458e-01, +9.46267243583929329e-01, /* pole */
      -1.35361765182335225e-01, +9.33815059006228299e-01, /* pole */
      +2.56644910774320341e-02, +8.87770842524976067e-01, /* pole */
      +2.94714103170947206e-01, +7.29579291033553901e-01, /* pole */
      +5.83809720009322852e-01, +3.09862657111590167e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.19741662039793599e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.00057570088471671629,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -107.52488334779183,
    };
    static const double zeros[] = {
      -3.05248149879130626e-01, +9.52272842727003188e-01,
      -3.04671082450923170e-01, +9.52457627151036434e-01,
      -3.03342777458647461e-01, +9.52881503317003942e-01,
      -3.00861492351649606e-01, +9.53667847009606495e-01,
      -2.96473685335889858e-01, +9.55041022104996196e-01,
      -2.88837188398592570e-01, +9.57378231733934881e-01,
      -2.75587818776128424e-01, +9.61275899074878426e-01,
      -2.52544085809507324e-01, +9.67585388853428374e-01,
      -2.12222636045612478e-01, +9.77221342762145895e-01,
      -1.41046801579312042e-01, +9.90002929169528367e-01,
      -1.44484542883441900e-02, +9.99895615636291146e-01,
      +2.08145342846356468e-01, +9.78097907293218105e-01,
      +5.62718002648720517e-01, +8.26648927595647787e-01,
      +9.33963089385106082e-01, +3.57369483400902965e-01,
    };
    static const double poles[] = {
      -3.08249269334604226e-01, +9.50871119947562238e-01, /* pole */
      -3.08318947298870105e-01, +9.49849785800539359e-01, /* pole */
      -3.08732929526483513e-01, +9.48288205594921330e-01, /* pole */
      -3.09612238702754727e-01, +9.45720980590107563e-01, /* pole */
      -3.11211401608507743e-01, +9.41387627992279041e-01, /* pole */
      -3.13982843263707068e-01, +9.34017669533730777e-01, /* pole */
      -3.18678300223372124e-01, +9.21488327335335589e-01, /* pole */
      -3.26477970638839154e-01, +9.00300192503019403e-01, /* pole */
      -3.39084090413043049e-01, +8.64846885953690525e-01, /* pole */
      -3.58572962941428319e-01, +8.06610970386091641e-01, /* pole */
      -3.86539526148252321e-01, +7.13884164271809563e-01, /* pole */
      -4.21931535846647610e-01, +5.73560227976588344e-01, /* pole */
      -4.57977998631928784e-01, +3.77358710835551170e-01, /* pole */
      -4.81826522376316624e-01, +1.32420569267938798e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.96587404478231293e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              28,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.00057570088471671629,
      /* passband_edge = */      30,
      /* passband_edge2 = */     45.648794107172911,
      /* stopband_edge = */      0,
      /* stopband_db = */        -83.549144522021422,
    };
    static const double zeros[] = {
      -9.62913802209075520e-01, +2.69809209470806499e-01,
      -3.08348469841621453e-01, +9.51273473374681378e-01,
      -9.62925412357056087e-01, +2.69767770938252660e-01,
      -3.08204117760003782e-01, +9.51320251963437280e-01,
      -9.62953512331847339e-01, +2.69667449069699083e-01,
      -3.07854500795832264e-01, +9.51433448192646303e-01,
      -9.63009899772309330e-01, +2.69466014444357860e-01,
      -3.07151897284804298e-01, +9.51660502487281512e-01,
      -9.63118194690107399e-01, +2.69078693056266638e-01,
      -3.05798617321705268e-01, +9.52096216589549016e-01,
      -9.63323557640059502e-01, +2.68342548425885896e-01,
      -3.03218193441471495e-01, +9.52921154747910060e-01,
      -9.63710889835335038e-01, +2.66948161283772023e-01,
      -2.98300206975122995e-01, +9.54472098344733566e-01,
      -9.64437654986091131e-01, +2.64310441800791107e-01,
      -2.88887973867898218e-01, +9.57362908491080189e-01,
      -9.65789656062978508e-01, +2.59326705608492303e-01,
      -2.70709160500788193e-01, +9.62661181528038146e-01,
      -9.68263471773524009e-01, +2.49931689125413026e-01,
      -2.34995288235382266e-01, +9.71996509513881723e-01,
      -9.72639458872872731e-01, +2.32319786164427527e-01,
      -1.62717252013431846e-01, +9.86672740019302053e-01,
      -9.79833963115173900e-01, +1.99813424789257066e-01,
      -9.80500770567153089e-03, +9.99951929756571656e-01,
      -9.89798183853442537e-01, +1.42476507679079795e-01,
      +3.21592092698014920e-01, +9.46878305757456684e-01,
      -9.98582422678730985e-01, +5.32272967289925611e-02,
      +8.67387589690159500e-01, +4.97633167354724848e-01,
    };
    static const double poles[] = {
      -9.62846120650200588e-01, +2.69949449800750862e-01, /* pole */
      -3.08832688347737694e-01, +9.51015177830323966e-01, /* pole */
      -9.62776624723100438e-01, +2.69952303136977545e-01, /* pole */
      -3.08832845972284831e-01, +9.50770174626898501e-01, /* pole */
      -9.62660988470025569e-01, +2.69973952476207979e-01, /* pole */
      -3.08892699291770823e-01, +9.50360281203972423e-01, /* pole */
      -9.62450530869599419e-01, +2.70023478309582954e-01, /* pole */
      -3.09037515018029063e-01, +9.49613473160367372e-01, /* pole */
      -9.62056666185144405e-01, +2.70121609107846938e-01, /* pole */
      -3.09328510683306301e-01, +9.48217305189890558e-01, /* pole */
      -9.61313655224396935e-01, +2.70309219823222535e-01, /* pole */
      -3.09889077364170096e-01, +9.45591046666126700e-01, /* pole */
      -9.59908999703745436e-01, +2.70663732647053845e-01, /* pole */
      -3.10958312700726491e-01, +9.40654244383660743e-01, /* pole */
      -9.57252496046163692e-01, +2.71328890951783863e-01, /* pole */
      -3.12997171888034664e-01, +9.31417735229313148e-01, /* pole */
      -9.52229924607924438e-01, +2.72565671543889665e-01, /* pole */
      -3.16901020461125282e-01, +9.14304680698579730e-01, /* pole */
      -9.42741271661940994e-01, +2.74831130303315685e-01, /* pole */
      -3.24435624458730976e-01, +8.83174552109357136e-01, /* pole */
      -9.24839035130332165e-01, +2.78882067376411136e-01, /* pole */
      -3.39151315344253745e-01, +8.28408040031870740e-01, /* pole */
      -8.91117087422088505e-01, +2.85922540913393919e-01, /* pole */
      -3.68291519635181419e-01, +7.37606870836072681e-01, /* pole */
      -8.27599365225940686e-01, +2.98479937870161749e-01, /* pole */
      -4.26575842891761425e-01, +6.01915846572339097e-01, /* pole */
      -7.09113420432266439e-01, +3.29690598992270045e-01, /* pole */
      -5.41302821556789215e-01, +4.37546218493750083e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = -4.08837156330568785e+00;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              28,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 2.3037522648656914,
      /* passband_edge = */      30,
      /* passband_edge2 = */     45.648794107172911,
      /* stopband_edge = */      0,
      /* stopband_db = */        -83.549144522021422,
    };
    static const double zeros[] = {
      -9.62858174188529259e-01, +2.70007659888255647e-01,
      -3.09039296605879255e-01, +9.51049269571952016e-01,
      -9.62857392399378398e-01, +2.70010447764284733e-01,
      -3.09048995843044683e-01, +9.51046117792615719e-01,
      -9.62855179363747982e-01, +2.70018339326062617e-01,
      -3.09076450350406928e-01, +9.51037195822956427e-01,
      -9.62849696477041772e-01, +2.70037889923004093e-01,
      -3.09144460889217154e-01, +9.51015090469922719e-01,
      -9.62836387471147415e-01, +2.70085340145499953e-01,
      -3.09309493667036306e-01, +9.50961427781085944e-01,
      -9.62804186369338844e-01, +2.70200108641124259e-01,
      -3.09708473268661577e-01, +9.50831563204332597e-01,
      -9.62726282671170885e-01, +2.70477549260837613e-01,
      -3.10671869432168957e-01, +9.50517222118316685e-01,
      -9.62537601573630774e-01, +2.71148235393266546e-01,
      -3.12994402755221446e-01, +9.49754970423372979e-01,
      -9.62079294829581988e-01, +2.72769922205902904e-01,
      -3.18573016804660214e-01, +9.47898324169832995e-01,
      -9.60958329796617550e-01, +2.76693130370985751e-01,
      -3.31853778876557926e-01, +9.43330837747472728e-01,
      -9.58172084075132324e-01, +2.86192692602585119e-01,
      -3.62784024836246999e-01, +9.31873248528797160e-01,
      -9.50998888102444928e-01, +3.09194299475125278e-01,
      -4.30949895899412627e-01, +9.02375856959995004e-01,
      -9.31316983263036469e-01, +3.64209660341178965e-01,
      -5.61459336938757048e-01, +8.27504328063781935e-01,
      -8.74639325670464984e-01, +4.84774225790433833e-01,
      -7.40100352020679919e-01, +6.72496445298312207e-01,
    };
    static const double poles[] = {
      -9.62859627884477209e-01, +2.70000972929255290e-01, /* pole */
      -3.09015971019804325e-01, +9.51055345832683918e-01, /* pole */
      -9.62859272169769076e-01, +2.69997987046504684e-01, /* pole */
      -3.09005411130699237e-01, +9.51054522431544425e-01, /* pole */
      -9.62859046690730525e-01, +2.69989754188817488e-01, /* pole */
      -3.08976402561919328e-01, +9.51054909824606609e-01, /* pole */
      -9.62858763940677154e-01, +2.69969435854672812e-01, /* pole */
      -3.08904845969337427e-01, +9.51056825535685157e-01, /* pole */
      -9.62858187826986955e-01, +2.69920155769370063e-01, /* pole */
      -3.08731284670813622e-01, +9.51061835934252198e-01, /* pole */
      -9.62856832659201634e-01, +2.69800987246558932e-01, /* pole */
      -3.08311447580042641e-01, +9.51073957191157504e-01, /* pole */
      -9.62853531151993014e-01, +2.69512991353251907e-01, /* pole */
      -3.07296009071830856e-01, +9.51102405313477561e-01, /* pole */
      -9.62845298885679268e-01, +2.68817234555413509e-01, /* pole */
      -3.04838094799680825e-01, +9.51165815410380366e-01, /* pole */
      -9.62823898242876064e-01, +2.67137512746417949e-01, /* pole */
      -2.98876393477232827e-01, +9.51287470483147546e-01, /* pole */
      -9.62763577626970957e-01, +2.63088898927674619e-01, /* pole */
      -2.84346572326168667e-01, +9.51393851977364147e-01, /* pole */
      -9.62570467457709911e-01, +2.53371793113691013e-01, /* pole */
      -2.48559980480514225e-01, +9.50513713738869392e-01, /* pole */
      -9.61868173712654539e-01, +2.30323922305098711e-01, /* pole */
      -1.58757897156492384e-01, +9.41208065667558547e-01, /* pole */
      -9.59340141452308948e-01, +1.77651575159131631e-01, /* pole */
      +6.68280850253035047e-02, +8.71513696121980486e-01, /* pole */
      -9.53734031739392085e-01, +7.18651192542072714e-02, /* pole */
      +4.93807774790046450e-01, +4.59538039774052109e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.59854677196190798e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 1.8712387385982618,
      /* passband_edge = */      39599.999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -74.32024023695223,
    };
    static const double zeros[] = {
      -9.51069102004531985e-01, +3.08978256860079292e-01,
      -9.51075748397449128e-01, +3.08957797781884702e-01,
      -9.51095686232739657e-01, +3.08896415695414150e-01,
      -9.51148828189683382e-01, +3.08732743053587000e-01,
      -9.51288094470770340e-01, +3.08303359239192831e-01,
      -9.51651104405026871e-01, +3.07181014199596536e-01,
      -9.52589351354482972e-01, +3.04258981274251439e-01,
      -9.54963439380224988e-01, +2.96723489880884539e-01,
      -9.60650581458668551e-01, +2.77759716919358246e-01,
      -9.72504996806923039e-01, +2.32882011296636354e-01,
      -9.90012367382223979e-01, +1.40980539189791282e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.51053131445850020e-01, +3.09016973289489572e-01, /* pole */
      -9.51037661718051930e-01, +3.09033259158804863e-01, /* pole */
      -9.50997370124264219e-01, +3.09084122170692865e-01, /* pole */
      -9.50891880251492960e-01, +3.09220471536736985e-01, /* pole */
      -9.50615221334576543e-01, +3.09578847544218438e-01, /* pole */
      -9.49887693063853678e-01, +3.10518590262501504e-01, /* pole */
      -9.47961453381293229e-01, +3.12985089825895457e-01, /* pole */
      -9.42770049325617920e-01, +3.19480005745278406e-01, /* pole */
      -9.28115030573029576e-01, +3.36708678092527136e-01, /* pole */
      -8.81487645641474304e-01, +3.82733421638813132e-01, /* pole */
      -6.84786510980071705e-01, +4.88167930614242240e-01, /* pole */
      -4.32235489852729263e-02, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.63074659754293705e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              23,
      /* sampling_frequency = */ 88000.0000,
      /* passband_ripple_db = */ 0.3271612804781,
      /* passband_edge = */      39599.999999999993,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -74.32024023695223,
    };
    static const double zeros[] = {
      -9.51024196399663291e-01, +3.09116447091342450e-01,
      -9.51010224886891242e-01, +3.09159428387012880e-01,
      -9.50970727544551142e-01, +3.09280900401862902e-01,
      -9.50872973976660152e-01, +3.09581309773025115e-01,
      -9.50635535909724494e-01, +3.10309648360521995e-01,
      -9.50058116900612371e-01, +3.12073027529234381e-01,
      -9.48638694825541195e-01, +3.16361544249129101e-01,
      -9.45052107555927612e-01, +3.26919736333093680e-01,
      -9.35354188274019505e-01, +3.53712513884157087e-01,
      -9.04232090094466923e-01, +4.27041364791974076e-01,
      -7.44153022005662468e-01, +6.68009191433650784e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.51044113393007429e-01, +3.09016487705260079e-01, /* pole */
      -9.51023855670132812e-01, +3.08969565926368317e-01, /* pole */
      -9.50989370880600871e-01, +3.08844408873025333e-01, /* pole */
      -9.50912291746065619e-01, +3.08537889966485013e-01, /* pole */
      -9.50729565833963952e-01, +3.07798078039560608e-01, /* pole */
      -9.50293675596207144e-01, +3.06020612504892930e-01, /* pole */
      -9.49263771361144260e-01, +3.01774722303576604e-01, /* pole */
      -9.46895558602654841e-01, +2.91762573487650945e-01, /* pole */
      -9.41793088344184293e-01, +2.68855723453710671e-01, /* pole */
      -9.32344294549842800e-01, +2.19920278316212414e-01, /* pole */
      -9.19959059688999936e-01, +1.29393885554034888e-01, /* pole */
      -9.13449700891869831e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.20975379016804541e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 0.029936361692099563,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -101.06490812048006,
    };
    static const double zeros[] = {
      -8.00014211763783512e-01, +5.99981050514074243e-01,
      -9.17873400048854537e-01, +3.96873306588835073e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +4.13855965082556787e-01, +7.48989503628663233e-01, /* pole */
      +4.44342258135604629e-01, +4.13387760880290833e-01, /* pole */
      +4.80721508433785694e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.15768625656146905e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 4.4068519161136024,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -101.06490812048006,
    };
    static const double zeros[] = {
      +9.56920789442312625e-01, +2.90349104929051138e-01,
      +9.82868516167026640e-01, +1.84308111399437258e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.43359058406073014e-01, +7.97635589112496879e-01, /* pole */
      +1.56042709883435882e-01, +8.18231097502909233e-01, /* pole */
      -5.61117595570488348e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.55959816117056321e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              5,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 4.4068519161136024,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     102496.71983316119,
      /* stopband_edge = */      0,
      /* stopband_db = */        -141.37591259615752,
    };
    static const double zeros[] = {
      -9.95906462266277770e-01, +9.03898136753633480e-02,
      +9.89882356394831353e-01, +1.41890522933055624e-01,
      -9.98421465655388918e-01, +5.61656204323008473e-02,
      +9.96091224773304917e-01, +8.83304699954527983e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -7.85740114789403199e-01, +5.87968176272481546e-01, /* pole */
      +5.54722052011740518e-01, +8.01243154909687894e-01, /* pole */
      -6.22937280068785237e-01, +6.89079339734812391e-01, /* pole */
      +3.11456694008223567e-01, +8.55960564491915266e-01, /* pole */
      -1.95644566475476844e-01, +8.45676064795428339e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.33830258937214664e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              5,
      /* sampling_frequency = */ 256000.0000,
      /* passband_ripple_db = */ 2.1143993217082766,
      /* passband_edge = */      38400,
      /* passband_edge2 = */     102496.71983316119,
      /* stopband_edge = */      0,
      /* stopband_db = */        -141.37591259615752,
    };
    static const double zeros[] = {
      -3.23531874740633274e-01, +9.46217272103406004e-01,
      -1.17810494017543357e-01, +9.93036095768599081e-01,
      -2.86086307397941553e-01, +9.58203853425465213e-01,
      -1.58305870724553210e-01, +9.87390121124442355e-01,
      -2.23154519771926491e-01, +9.74783083719327204e-01,
    };
    static const double poles[] = {
      -7.94072785585076324e-01, +5.61033825492564864e-01, /* pole */
      +5.76767584928925880e-01, +7.69621108834647405e-01, /* pole */
      -8.19918794213228996e-01, +3.78777568338567794e-01, /* pole */
      +6.75216816763764771e-01, +5.33456715555002847e-01, /* pole */
      +7.67391554429158851e-01, +0.00000000000000000e+00, /* pole */
      -8.45719275278123517e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.82860207862908554e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0.0083916073789581345,
      /* passband_edge = */      13230,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -171.4596564851017,
    };
    static const double zeros[] = {
      -7.14889834940706015e-01, +6.99237101345781098e-01,
      -7.38421997829392440e-01, +6.74338900792212037e-01,
      -7.84389711321385130e-01, +6.20268313533066218e-01,
      -8.48437073149964416e-01, +5.29296261941005874e-01,
      -9.19342421765634299e-01, +3.93458398743880555e-01,
      -9.77275672255511263e-01, +2.11972310497240302e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.26730414187378082e-01, +9.17466071396538796e-01, /* pole */
      -2.73261899911396877e-01, +8.76496811636202966e-01, /* pole */
      -1.79678826671843789e-01, +8.28700193910530847e-01, /* pole */
      -4.10162997148606490e-02, +7.50056644719834353e-01, /* pole */
      +1.34747161397761078e-01, +6.03159994320674353e-01, /* pole */
      +3.02260193983018954e-01, +3.49413819728614461e-01, /* pole */
      +3.75265303234726844e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.88480433845004567e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0.00096436489717618805,
      /* passband_edge = */      13230,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -171.4596564851017,
    };
    static const double zeros[] = {
      +3.16606395643803107e-01, +9.48557004210837995e-01,
      +3.62014913466373489e-01, +9.32172302971899125e-01,
      +4.55417620356347586e-01, +8.90277929114813960e-01,
      +5.97137176401199010e-01, +8.02139135412057724e-01,
      +7.72359174449970154e-01, +6.35186040182685874e-01,
      +9.32444244081184426e-01, +3.61313896328204542e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -2.51422036278881711e-01, +9.32337456433607215e-01, /* pole */
      -2.68503906403903814e-01, +8.54917978361405928e-01, /* pole */
      -3.13705664120697114e-01, +7.57250553589644548e-01, /* pole */
      -3.78838235000082735e-01, +6.27591781602008569e-01, /* pole */
      -4.50260280803991719e-01, +4.56236429846228198e-01, /* pole */
      -5.07582797907974448e-01, +2.42201968341042684e-01, /* pole */
      -5.29791066178376679e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.82851485436558593e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              13,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 0.00096436489717618805,
      /* passband_edge = */      13230,
      /* passband_edge2 = */     17689.17861442751,
      /* stopband_edge = */      0,
      /* stopband_db = */        -149.94324426459934,
    };
    static const double zeros[] = {
      -8.78472056025334624e-01, +4.77793728278868679e-01,
      -8.63634832808548719e-02, +9.96263694387985566e-01,
      -8.84582351084568064e-01, +4.66384030761879731e-01,
      -5.90930041299197531e-02, +9.98252481521033519e-01,
      -8.97712389491349572e-01, +4.40581962583276199e-01,
      +4.69385616448609894e-03, +9.99988983796475162e-01,
      -9.19363963661683159e-01, +3.93408060822702799e-01,
      +1.28572119767998455e-01, +9.91700161348360143e-01,
      -9.50011455978003738e-01, +3.12215043696733807e-01,
      +3.59476239921413310e-01, +9.33154238554357418e-01,
      -9.83550856770937010e-01, +1.80631426239055787e-01,
      +7.35484577328119893e-01, +6.77541464791990844e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.13287440235386239e-01, +5.68668163623336942e-01, /* pole */
      -2.87909882471432288e-01, +9.44403484229524026e-01, /* pole */
      -7.95040639137710725e-01, +5.66571152879581708e-01, /* pole */
      -2.94683577394243557e-01, +9.15095137245833579e-01, /* pole */
      -7.68743400479633632e-01, +5.70657575155087970e-01, /* pole */
      -3.15523863833114815e-01, +8.78666823500749983e-01, /* pole */
      -7.31577272137058365e-01, +5.81464838275933427e-01, /* pole */
      -3.51294939536577477e-01, +8.33638576515114638e-01, /* pole */
      -6.81057203517500653e-01, +6.00547120437613380e-01, /* pole */
      -4.02712968266716043e-01, +7.80795779878496243e-01, /* pole */
      -6.16807999047259581e-01, +6.30230489809947225e-01, /* pole */
      -4.68442631883622729e-01, +7.24524272209181142e-01, /* pole */
      -5.42963927163384485e-01, +6.72187394890136569e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.96983321420811193e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              13,
      /* sampling_frequency = */ 44100.0000,
      /* passband_ripple_db = */ 1.4752585036164892e-05,
      /* passband_edge = */      13230,
      /* passband_edge2 = */     17689.17861442751,
      /* stopband_edge = */      0,
      /* stopband_db = */        -149.94324426459934,
    };
    static const double zeros[] = {
      -7.40361310325429178e-01, +6.72209141691195389e-01,
      -4.65519996182863327e-01, +8.85037362575110120e-01,
      -7.35391916912018884e-01, +6.77642035694706824e-01,
      -4.74021507467322856e-01, +8.80513265350617358e-01,
      -7.25063935391331071e-01, +6.88681558918805004e-01,
      -4.90997337723800997e-01, +8.71161072562439243e-01,
      -7.08737217282538801e-01, +7.05472576957179354e-01,
      -5.16072792133777325e-01, +8.56544729257758597e-01,
      -6.85845224882348559e-01, +7.27747433871175353e-01,
      -5.48019124890181675e-01, +8.36465802501572320e-01,
      -6.56428295059631073e-01, +7.54388423456448631e-01,
      -5.84389199003046755e-01, +8.11473514101709537e-01,
      -6.21721876314884070e-01, +7.83238091841491069e-01,
    };
    static const double poles[] = {
      -7.90955807979301317e-01, +5.95415386823680559e-01, /* pole */
      -3.39953373134998960e-01, +9.23904947102316743e-01, /* pole */
      -7.76576519548678235e-01, +5.78175794812289201e-01, /* pole */
      -3.22023171169936728e-01, +8.94108560314830880e-01, /* pole */
      -7.59803745065231184e-01, +5.52910968152580562e-01, /* pole */
      -2.95586095175500441e-01, +8.55363478913778730e-01, /* pole */
      -7.34437236147818751e-01, +5.16125433145071644e-01, /* pole */
      -2.62081458128633316e-01, +7.95067203258850608e-01, /* pole */
      -6.88622530565374813e-01, +4.64634993778370142e-01, /* pole */
      -2.33780348042118918e-01, +6.94394119343413552e-01, /* pole */
      -5.96587431498706189e-01, +4.01566277296687535e-01, /* pole */
      -2.53376900303719066e-01, +5.37774150983137300e-01, /* pole */
      -4.08802127490818235e-01, +3.84636371276163957e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.37790352843335584e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.00034133924194338825,
      /* passband_edge = */      28799.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -121.67477506802241,
    };
    static const double zeros[] = {
      -9.87851872942535736e-01, +1.55398446336905943e-01,
      -9.89855072039783179e-01, +1.42080738867433165e-01,
      -9.93332583783878009e-01, +1.15283901708976275e-01,
      -9.97129001407006887e-01, +7.57215593676279358e-02,
      -9.99649608729067718e-01, +2.64699786139283959e-02,
    };
    static const double poles[] = {
      -9.43715943204903596e-01, +2.72244259550320333e-01, /* pole */
      -9.00877263934273254e-01, +2.69927439133560376e-01, /* pole */
      -8.34583193177121596e-01, +2.65605230284234883e-01, /* pole */
      -7.30173412424142687e-01, +2.30344582522724217e-01, /* pole */
      -6.16466038510419256e-01, +1.02700025762799768e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.87030317442550953e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              10,
      /* sampling_frequency = */ 64000.0000,
      /* passband_ripple_db = */ 0.028347842613885146,
      /* passband_edge = */      28799.999999999996,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -121.67477506802241,
    };
    static const double zeros[] = {
      -8.67152782649371234e-01, +4.98042218635581402e-01,
      -8.44682658136373332e-01, +5.35267416385184713e-01,
      -7.76861043623220393e-01, +6.29672072511272218e-01,
      -5.55738159530976161e-01, +8.31357382863184280e-01,
      +3.95443592432307200e-01, +9.18490264076996854e-01,
    };
    static const double poles[] = {
      -9.36521309431312510e-01, +3.14343887116259213e-01, /* pole */
      -9.19595738214701308e-01, +2.84745603792155244e-01, /* pole */
      -9.06414953376305399e-01, +2.31285488637625936e-01, /* pole */
      -8.96341945067249957e-01, +1.52787342176679525e-01, /* pole */
      -8.90541783425973565e-01, +5.36641507888085192e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.09842371397489955e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.00040467563607611491,
      /* passband_edge = */      300,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -163.59156352558358,
    };
    static const double zeros[] = {
      +4.52761382584023930e-01, +8.91631723549921151e-01,
      +4.40729525686681889e-01, +8.97639952981145073e-01,
      +4.14163504378830150e-01, +9.10202500348492038e-01,
      +3.67291682364225380e-01, +9.30105811220452994e-01,
      +2.89252267303137522e-01, +9.57252905903133944e-01,
      +1.60805095222686129e-01, +9.86986180931842094e-01,
      -4.93119245463460171e-02, +9.98783427023864578e-01,
      -3.75845750126472034e-01, +9.26682239018245979e-01,
      -7.81866779176412452e-01, +6.23445538616087203e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.70020263550731987e-01, +8.10213396055122215e-01, /* pole */
      +5.66440811131042277e-01, +7.88547311438566378e-01, /* pole */
      +5.69074374917063253e-01, +7.57874510391280731e-01, /* pole */
      +5.77467496354408083e-01, +7.14573858401410411e-01, /* pole */
      +5.91305671204249150e-01, +6.54233735587913423e-01, /* pole */
      +6.09837680826449180e-01, +5.72124148774995067e-01, /* pole */
      +6.31194975669701841e-01, +4.64324275742203474e-01, /* pole */
      +6.51919383363982363e-01, +3.29620708189504896e-01, /* pole */
      +6.67314904811980525e-01, +1.71669001205571714e-01, /* pole */
      +6.73045695504228103e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.11543529848209138e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.60283636152516495,
      /* passband_edge = */      300,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -163.59156352558358,
    };
    static const double zeros[] = {
      +6.47548310424469697e-01, +7.62024399652934115e-01,
      +6.53808938682476137e-01, +7.56659680238675136e-01,
      +6.67407801339962137e-01, +7.44692437661721884e-01,
      +6.90471834214323898e-01, +7.23359278752064738e-01,
      +7.25916463038091209e-01, +6.87782879032523997e-01,
      +7.76437562452611862e-01, +6.30194185638558890e-01,
      +8.41891379476093271e-01, +5.39647019044709131e-01,
      +9.14619191312665003e-01, +4.04316379686213923e-01,
      +9.75595702499947870e-01, +2.19574646222265607e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      +5.86610244771926359e-01, +8.07101824903464937e-01, /* pole */
      +5.76805715603773916e-01, +8.07997260707822851e-01, /* pole */
      +5.57447570025200512e-01, +8.13303842191130610e-01, /* pole */
      +5.23595895932004951e-01, +8.23389818035751730e-01, /* pole */
      +4.66158759587856408e-01, +8.38257659176973591e-01, /* pole */
      +3.68943142714470540e-01, +8.54787979187252800e-01, /* pole */
      +2.04848590215759269e-01, +8.57772541245765852e-01, /* pole */
      -5.92689615500832101e-02, +7.94567886534208845e-01, /* pole */
      -4.00765312579379307e-01, +5.38223375606196153e-01, /* pole */
      -5.89977804309762099e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.59381758762457185e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              19,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 0.60283636152516495,
      /* passband_edge = */      300,
      /* passband_edge2 = */     864.72416676909324,
      /* stopband_edge = */      0,
      /* stopband_db = */        -141.79901550810578,
    };
    static const double zeros[] = {
      -9.18133194173124334e-01, +3.96271924008572585e-01,
      +6.15443620936056424e-01, +7.88180911624428826e-01,
      -9.19129191749390118e-01, +3.93956252487649539e-01,
      +6.19391447684914831e-01, +7.85082310674992456e-01,
      -9.21372662165287348e-01, +3.88680353779878474e-01,
      +6.28339747420315620e-01, +7.77939047619910951e-01,
      -9.25398752743161412e-01, +3.78994918727680596e-01,
      +6.44595024373156411e-01, +7.64524201417698723e-01,
      -9.32055854270810835e-01, +3.62314620902205786e-01,
      +6.72041717526763116e-01, +7.40513288134438841e-01,
      -9.42415165705634772e-01, +3.34445295150075650e-01,
      +7.16222038286860929e-01, +6.97872475365101774e-01,
      -9.57259206953283814e-01, +2.89231413755785260e-01,
      +7.82855200408660412e-01, +6.22203933765382189e-01,
      -9.75648492579808457e-01, +2.19339961992217586e-01,
      +8.71349852241874445e-01, +4.90662241259772813e-01,
      -9.92713072100574911e-01, +1.20502101561088121e-01,
      +9.60023920194766456e-01, +2.79917974867411379e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.10476034596350292e-01, +4.11988442610732419e-01, /* pole */
      +5.87118932206769695e-01, +8.07925824520585478e-01, /* pole */
      -9.08096774365440229e-01, +4.13625145146215067e-01, /* pole */
      +5.81485702299882501e-01, +8.08405927845158856e-01, /* pole */
      -9.03801262401948358e-01, +4.17892514231841428e-01, /* pole */
      +5.70092938848092756e-01, +8.11462291926049462e-01, /* pole */
      -8.96238429058574981e-01, +4.25990932535037725e-01, /* pole */
      +5.49619304118956653e-01, +8.17663420402165331e-01, /* pole */
      -8.82897276327401004e-01, +4.40168845517568963e-01, /* pole */
      +5.14074901922361160e-01, +8.27904090525080960e-01, /* pole */
      -8.58999019863481816e-01, +4.64224415047471028e-01, /* pole */
      +4.53175578458704653e-01, +8.42625273637051864e-01, /* pole */
      -8.15182251831924276e-01, +5.04019439966940430e-01, /* pole */
      +3.50392887883681781e-01, +8.59288192247528060e-01, /* pole */
      -7.33250607795643883e-01, +5.66918921463302472e-01, /* pole */
      +1.83812035425639703e-01, +8.66481343666887338e-01, /* pole */
      -5.83143347859223504e-01, +6.56666174330710395e-01, /* pole */
      -5.90782260449546279e-02, +8.39169715211779632e-01, /* pole */
      -3.43446979339703196e-01, +7.60155259946361905e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.92323009703125570e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              19,
      /* sampling_frequency = */ 2000.0000,
      /* passband_ripple_db = */ 4.3725653915046623,
      /* passband_edge = */      300,
      /* passband_edge2 = */     864.72416676909324,
      /* stopband_edge = */      0,
      /* stopband_db = */        -141.79901550810578,
    };
    static const double zeros[] = {
      -9.05800536532379130e-01, +4.23704363935107964e-01,
      +5.67787509286387104e-01, +8.23175160156306762e-01,
      -9.04899314303966085e-01, +4.25625693505704505e-01,
      +5.64391614444980094e-01, +8.25507180795048479e-01,
      -9.02776689108632180e-01, +4.30109578598356690e-01,
      +5.56438631019707919e-01, +8.30888710904723493e-01,
      -8.98652754342487037e-01, +4.38660719819613965e-01,
      +5.41166518427734888e-01, +8.40915453141874658e-01,
      -8.90912492630545394e-01, +4.54174999834676407e-01,
      +5.13124959092271848e-01, +8.58313914810050371e-01,
      -8.76195121865976589e-01, +4.81956542043228631e-01,
      +4.61942059878310163e-01, +8.86910104416103517e-01,
      -8.47102144008294533e-01, +5.31430106050222339e-01,
      +3.68280035163247088e-01, +9.29714910980864895e-01,
      -7.86387140566734422e-01, +6.17733976037642685e-01,
      +1.99798618805445277e-01, +9.79836982320751226e-01,
      -6.55795365337913383e-01, +7.54938698704280875e-01,
      -7.38349887700436025e-02, +9.97270472055263313e-01,
      -4.05057943850069713e-01, +9.14291016101522080e-01,
    };
    static const double poles[] = {
      -9.10926245328982143e-01, +4.12034738684016910e-01, /* pole */
      +5.87841723087966228e-01, +8.08441294430548241e-01, /* pole */
      -9.11311686593633707e-01, +4.09936374841306517e-01, /* pole */
      +5.90532249454016833e-01, +8.05231373354148072e-01, /* pole */
      -9.12591950165133170e-01, +4.05247772692984953e-01, /* pole */
      +5.97247562906806584e-01, +7.98425243280846075e-01, /* pole */
      -9.15122179053230567e-01, +3.96480943866454438e-01, /* pole */
      +6.09998811226859172e-01, +7.85678869748850039e-01, /* pole */
      -9.19533908320313897e-01, +3.80927206864064949e-01, /* pole */
      +6.32352438387783988e-01, +7.62502959128478430e-01, /* pole */
      -9.26691093803490462e-01, +3.54040838144108305e-01, /* pole */
      +6.69673346594737984e-01, +7.20541921640834415e-01, /* pole */
      -9.37323107817060075e-01, +3.08850790898783145e-01, /* pole */
      +7.28018187726737209e-01, +6.44701645586987171e-01, /* pole */
      -9.50889323087774740e-01, +2.36527179885715338e-01, /* pole */
      +8.08198503136403179e-01, +5.10638734095206104e-01, /* pole */
      -9.63735254862348345e-01, +1.31071875966208340e-01, /* pole */
      +8.90936923469552999e-01, +2.92591953061798637e-01, /* pole */
      +9.28919747228696746e-01, +0.00000000000000000e+00, /* pole */
      -9.69272929930491745e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.55133055675891703e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.0055714685957093035,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -100.94244310615012,
    };
    static const double zeros[] = {
      -6.97602472825983200e-01, +7.16485024202930143e-01,
      -7.48401077462321318e-01, +6.63246430260455044e-01,
      -8.43432279914439698e-01, +5.37235506271067531e-01,
      -9.50071844659703624e-01, +3.12031232386932533e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.50131706229182826e-01, +8.81305638963823412e-01, /* pole */
      -2.56888710264732534e-01, +7.89903697913898695e-01, /* pole */
      -1.04886637067678118e-01, +6.49891563078754175e-01, /* pole */
      +7.29555948361356871e-02, +3.92187174438675679e-01, /* pole */
      +1.61524881300593698e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.34216637531503465e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.00071487123272088884,
      /* passband_edge = */      30,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -100.94244310615012,
    };
    static const double zeros[] = {
      +3.07906059282876954e-01, +9.51416763914158414e-01,
      +4.04608202538934314e-01, +9.14490132499095165e-01,
      +6.04122863241034591e-01, +7.96891188374833415e-01,
      +8.63153735443578096e-01, +5.04941213399933808e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -1.88534684816839715e-01, +9.13219505424103795e-01, /* pole */
      -2.12691024127591194e-01, +7.63907760603899200e-01, /* pole */
      -2.67499791693728728e-01, +5.68923474736408030e-01, /* pole */
      -3.25272452402752377e-01, +3.09557747126022964e-01, /* pole */
      -3.50605150350280836e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +3.14691637160775285e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.00071487123272088884,
      /* passband_edge = */      30,
      /* passband_edge2 = */     47.135589758824779,
      /* stopband_edge = */      0,
      /* stopband_db = */        -53.247256054753471,
    };
    static const double zeros[] = {
      -9.88250260998482677e-01, +1.52844436066318395e-01,
      -1.57909555706142013e-01, +9.87453579778152846e-01,
      -9.89656654401149583e-01, +1.43456287417190242e-01,
      -9.48586225324221932e-02, +9.95490754216859286e-01,
      -9.92837854235843920e-01, +1.19469641316801045e-01,
      +8.91834054456325659e-02, +9.96015220864179374e-01,
      -9.97410718061287116e-01, +7.19156415285817086e-02,
      +5.36538108996587715e-01, +8.43876091375010851e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.80294407326944728e-01, +1.68441878481929763e-01, /* pole */
      -2.50038542583998158e-01, +9.37137962305765959e-01, /* pole */
      -9.65609194044012020e-01, +1.67698061990358777e-01, /* pole */
      -2.45662204652941668e-01, +8.58774655927998731e-01, /* pole */
      -9.35955522529906658e-01, +1.65754983457300720e-01, /* pole */
      -2.53913650955713721e-01, +7.18961245282168537e-01, /* pole */
      -8.68777367784743682e-01, +1.51910587102280503e-01, /* pole */
      -2.97970179327472662e-01, +4.71248615444423369e-01, /* pole */
      -4.50608769067611936e-01, +0.00000000000000000e+00, /* pole */
      -7.04889044820595445e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.50115682042890174e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              9,
      /* sampling_frequency = */ 100.0000,
      /* passband_ripple_db = */ 0.00052276005177062028,
      /* passband_edge = */      30,
      /* passband_edge2 = */     47.135589758824779,
      /* stopband_edge = */      0,
      /* stopband_db = */        -53.247256054753471,
    };
    static const double zeros[] = {
      -9.78038416884364659e-01, +2.08424698868261760e-01,
      -4.41867136645640424e-01, +8.97080505613951473e-01,
      -9.75567648498731121e-01, +2.19699256263320458e-01,
      -4.84218508612566323e-01, +8.74947104639487327e-01,
      -9.67975131076911577e-01, +2.51046102572089036e-01,
      -5.82149416235435146e-01, +8.13081826864149093e-01,
      -9.45266432171128601e-01, +3.26299513040496370e-01,
      -7.34989300987211491e-01, +6.78078702979477366e-01,
      -8.76952201123030850e-01, +4.80577607619696812e-01,
    };
    static const double poles[] = {
      -9.76082452146847190e-01, +1.88565454672181276e-01, /* pole */
      -3.51542920313423990e-01, +9.05646280943024240e-01, /* pole */
      -9.61218079297700534e-01, +1.82925906487062029e-01, /* pole */
      -3.19635105554177956e-01, +8.38232841495216818e-01, /* pole */
      -9.33511762756283558e-01, +1.69094526410732526e-01, /* pole */
      -2.70000412126580047e-01, +7.11625781070748253e-01, /* pole */
      -8.78002152405880532e-01, +1.31682696875406569e-01, /* pole */
      -2.25884565759257416e-01, +4.59052310178718448e-01, /* pole */
      -2.46566116839049526e-01, +0.00000000000000000e+00, /* pole */
      -8.04296450755184411e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.27709312150459386e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.065303251311236557,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -65.10189073841299,
    };
    static const double zeros[] = {
      -9.84098124515191408e-01, +1.77625677551650540e-01,
      -9.90691175516666633e-01, +1.36128596383732425e-01,
      -9.98631334295225326e-01, +5.23016076592098395e-02,
    };
    static const double poles[] = {
      -9.32216790375306470e-01, +2.82211240759489501e-01, /* pole */
      -8.39420384797820662e-01, +2.86622663898204844e-01, /* pole */
      -6.23109568841741068e-01, +1.89278219522556607e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.54583105265337495e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              6,
      /* sampling_frequency = */ 12000.0000,
      /* passband_ripple_db = */ 0.39947145689941194,
      /* passband_edge = */      5399.9999999999991,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -65.10189073841299,
    };
    static const double zeros[] = {
      -8.84157731157539262e-01, +4.67188512738008654e-01,
      -8.14225269185097078e-01, +5.80549059960014269e-01,
      -1.92479818526219398e-01, +9.81300932161033646e-01,
    };
    static const double poles[] = {
      -9.32078175126655917e-01, +3.06892889092118371e-01, /* pole */
      -9.08293159882947454e-01, +2.34696115289839857e-01, /* pole */
      -8.91493445831821552e-01, +9.10617010785720604e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +8.64673265239987682e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.07188874077178925,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -160.75999731221958,
    };
    static const double zeros[] = {
      +5.48878484418854784e-01, +8.35902152971303769e-01,
      +5.44678243120893968e-01, +8.38645104600710289e-01,
      +5.35314548922146471e-01, +8.44652788850116076e-01,
      +5.18575893287694489e-01, +8.55031603451515521e-01,
      +4.90296865877425825e-01, +8.71555496403283225e-01,
      +4.42869097378675858e-01, +8.96586282845659444e-01,
      +3.62153909689077291e-01, +9.32118310997544830e-01,
      +2.21306717485016763e-01, +9.75204253885311401e-01,
      -2.84427105111326149e-02, +9.99595424268628796e-01,
      -4.44561992483439583e-01, +8.95748086706945834e-01,
      -9.12742024102981331e-01, +4.08536408948324492e-01,
    };
    static const double poles[] = {
      +5.85635568022885189e-01, +8.08112807510428688e-01, /* pole */
      +5.86678954018157062e-01, +8.01953842562786789e-01, /* pole */
      +5.91224378553463659e-01, +7.91672403132829383e-01, /* pole */
      +5.99953822036413342e-01, +7.75289919380678150e-01, /* pole */
      +6.14063590239475654e-01, +7.49759181473265701e-01, /* pole */
      +6.35088779670649695e-01, +7.10621094738129844e-01, /* pole */
      +6.64375388280630141e-01, +6.51766540232142533e-01, /* pole */
      +7.01897161909636136e-01, +5.65736329224597601e-01, /* pole */
      +7.44356732149472733e-01, +4.45401287084171038e-01, /* pole */
      +7.83578176954958017e-01, +2.87897872433724500e-01, /* pole */
      +8.07818462720396480e-01, +9.99633373689674531e-02, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.70755131309589330e-06;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.0020276575209270883,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -160.75999731221958,
    };
    static const double zeros[] = {
      +6.39669717246871494e-01, +7.68649889635917072e-01,
      +6.44017933221051941e-01, +7.65010393190631577e-01,
      +6.53338126149506260e-01, +7.57066240773851873e-01,
      +6.68890634964343533e-01, +7.43360826555312437e-01,
      +6.92539906819339746e-01, +7.21379565459585370e-01,
      +7.26526337816405743e-01, +6.87138618081593378e-01,
      +7.72744236267361551e-01, +6.34717531911299981e-01,
      +8.31090689889044110e-01, +5.56136912261497640e-01,
      +8.96694653262267605e-01, +4.42649634373351419e-01,
      +9.57307490833502417e-01, +2.89071562063900944e-01,
      +9.94884794490862756e-01, +1.01016066498720136e-01,
    };
    static const double poles[] = {
      +5.89675569023437207e-01, +8.02782442655411099e-01, /* pole */
      +5.80744952126906910e-01, +7.98757447614991234e-01, /* pole */
      +5.65976979412133208e-01, +7.96117779300344552e-01, /* pole */
      +5.42781224208314428e-01, +7.93963659596537474e-01, /* pole */
      +5.07126084068312499e-01, +7.90936774430006095e-01, /* pole */
      +4.53021154811046545e-01, +7.84277851399810100e-01, /* pole */
      +3.72208211814829104e-01, +7.67837681893868917e-01, /* pole */
      +2.55360759816470051e-01, +7.28174371508919505e-01, /* pole */
      +9.86803359322483559e-02, +6.38900146303459260e-01, /* pole */
      -7.75725790483284966e-02, +4.60871122421215695e-01, /* pole */
      -2.06647333425028501e-01, +1.72225248201584108e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.02045673538884395e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              22,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 0.0020276575209270883,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     83916.38950757595,
      /* stopband_edge = */      0,
      /* stopband_db = */        -102.84789907074389,
    };
    static const double zeros[] = {
      -9.24517551946983152e-01, +3.81139470721620854e-01,
      +5.95261686867475937e-01, +8.03531906116793260e-01,
      -9.24811144825666620e-01, +3.80426532205943191e-01,
      +5.96567475883252651e-01, +8.02562923837305431e-01,
      -9.25495988921158363e-01, +3.78757408496318504e-01,
      +5.99620170806801855e-01, +8.00284731056154497e-01,
      -9.26796041850188823e-01, +3.75565036725762191e-01,
      +6.05441367491163063e-01, +7.95889911062095612e-01,
      -9.29121499312120958e-01, +3.69774579326373476e-01,
      +6.15940445322425867e-01, +7.87792718813782766e-01,
      -9.33156359713690597e-01, +3.59470733615260107e-01,
      +6.34425068328333008e-01, +7.72984367679314666e-01,
      -9.39931252078271440e-01, +3.41363796215376436e-01,
      +6.66250377782376169e-01, +7.45728123450390634e-01,
      -9.50708516733968723e-01, +3.10085981962256785e-01,
      +7.19018159264339318e-01, +6.94991285303723116e-01,
      -9.66198448666742760e-01, +2.57799452664236262e-01,
      +7.99846564454235520e-01, +6.00204526249808135e-01,
      -9.84384704335795169e-01, +1.76030548115175961e-01,
      +9.03125377497025372e-01, +4.29376935245543268e-01,
      -9.97987168922555012e-01, +6.34161703821940204e-02,
      +9.87050831169613896e-01, +1.60407782502515195e-01,
    };
    static const double poles[] = {
      -9.22645137021037343e-01, +3.84431021031513720e-01, /* pole */
      +5.88305423975598263e-01, +8.07418784833376924e-01, /* pole */
      -9.21429350157401017e-01, +3.84486689981770047e-01, /* pole */
      +5.86063988983659523e-01, +8.06194362939471443e-01, /* pole */
      -9.19444326408537127e-01, +3.84974115448885768e-01, /* pole */
      +5.81981400888470723e-01, +8.04910398178068864e-01, /* pole */
      -9.16001287067805259e-01, +3.86048394303147191e-01, /* pole */
      +5.74680470521588216e-01, +8.03099905031990802e-01, /* pole */
      -9.09897861894672588e-01, +3.88039518702104513e-01, /* pole */
      +5.61735880819084099e-01, +8.00063002784378918e-01, /* pole */
      -8.98980443924839556e-01, +3.91507605007810955e-01, /* pole */
      +5.38968373336080275e-01, +7.94523643590258954e-01, /* pole */
      -8.79336952343316436e-01, +3.97254920156853653e-01, /* pole */
      +4.99408715157253547e-01, +7.83913359315555724e-01, /* pole */
      -8.43789122024704019e-01, +4.06060119212869886e-01, /* pole */
      +4.32113909389606032e-01, +7.62854446673982944e-01, /* pole */
      -7.79115514265709574e-01, +4.17465802481254589e-01, /* pole */
      +3.21726591946616791e-01, +7.20457939737200515e-01, /* pole */
      -6.61006654070341093e-01, +4.26729622233977646e-01, /* pole */
      +1.49955810310354126e-01, +6.38406170572497933e-01, /* pole */
      -4.42845321433220307e-01, +4.28999514058028553e-01, /* pole */
      -1.08233844706471821e-01, +5.06466562577287949e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +4.97805445285997260e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              22,
      /* sampling_frequency = */ 192000.0000,
      /* passband_ripple_db = */ 4.3077194889761401,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     83916.38950757595,
      /* stopband_edge = */      0,
      /* stopband_db = */        -102.84789907074389,
    };
    static const double zeros[] = {
      -9.22597595694819472e-01, +3.85763757263611118e-01,
      +5.86765111352606894e-01, +8.09757188359178870e-01,
      -9.22524625271566645e-01, +3.85938227916794274e-01,
      +5.86443640107779718e-01, +8.09990035109776407e-01,
      -9.22334059650869675e-01, +3.86393429561044222e-01,
      +5.85604602148874775e-01, +8.10596847971948309e-01,
      -9.21908829509041405e-01, +3.87406905040771354e-01,
      +5.83734961756531612e-01, +8.11944268052371076e-01,
      -9.20984951161410881e-01, +3.89598151605232035e-01,
      +5.79685200886577157e-01, +8.14840516833256423e-01,
      -9.18975853974895562e-01, +3.94313808800949073e-01,
      +5.70936272300554437e-01, +8.20994380596814399e-01,
      -9.14545258687316731e-01, +4.04483584107623417e-01,
      +5.51917935442483243e-01, +8.33898430587866502e-01,
      -9.04462031476918837e-01, +4.26554139139037114e-01,
      +5.09997586900876265e-01, +8.60175831650298761e-01,
      -8.80032896212433080e-01, +4.74912730492621227e-01,
      +4.15652045109752288e-01, +9.09523709089587462e-01,
      -8.14226707848953901e-01, +5.80547042215748599e-01,
      +2.03091276395146542e-01, +9.79159809965763461e-01,
      -6.21728754248015991e-01, +7.83232632198895384e-01,
      -2.02819059564252363e-01, +9.79216232033289113e-01,
    };
    static const double poles[] = {
      -9.22817654553719002e-01, +3.85187065516732208e-01, /* pole */
      +5.87789814894493001e-01, +8.08963703955558500e-01, /* pole */
      -9.22843018412597194e-01, +3.84995952709965950e-01, /* pole */
      +5.88044052480928836e-01, +8.08648556656341944e-01, /* pole */
      -9.22944529797573576e-01, +3.84512536063013277e-01, /* pole */
      +5.88753978930221744e-01, +8.07891727457418218e-01, /* pole */
      -9.23183074781102619e-01, +3.83443807525520708e-01, /* pole */
      +5.90348025246872798e-01, +8.06231526809566890e-01, /* pole */
      -9.23699824157384497e-01, +3.81144438955144660e-01, /* pole */
      +5.93780245780764715e-01, +8.02652021248559389e-01, /* pole */
      -9.24791315253677193e-01, +3.76237574397246644e-01, /* pole */
      +6.01067008130486191e-01, +7.94948971023321405e-01, /* pole */
      -9.27042046593388736e-01, +3.65838994950732577e-01, /* pole */
      +6.16312769733328270e-01, +7.78323554860719691e-01, /* pole */
      -9.31477020810363543e-01, +3.44090061041089301e-01, /* pole */
      +6.47284887845943735e-01, +7.42243141145891738e-01, /* pole */
      -9.39401856445044592e-01, +2.99932945687516606e-01, /* pole */
      +7.05997104866458836e-01, +6.63752958911034807e-01, /* pole */
      -9.50811617997969760e-01, +2.16339471575086817e-01, /* pole */
      +7.99397413656292244e-01, +4.97843976493600460e-01, /* pole */
      -9.60745660331194018e-01, +8.12903779373284757e-02, /* pole */
      +8.91282016766640983e-01, +1.94053140543136854e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +6.68177161151099243e-04;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              27,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.08095839651852127,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -83.435580995768092,
    };
    static const double zeros[] = {
      -3.09277961029912951e-01, +9.50971683501238507e-01,
      -3.09359907881785745e-01, +9.50945028587548258e-01,
      -3.09573496289919259e-01, +9.50875517822830085e-01,
      -3.10048204521063020e-01, +9.50720837508711059e-01,
      -3.11071557534565835e-01, +9.50386493008512412e-01,
      -3.13262231675072944e-01, +9.49666664786098957e-01,
      -3.17939258523713908e-01, +9.48111084150686900e-01,
      -3.27893814643145154e-01, +9.44714584580320094e-01,
      -3.48949091197791161e-01, +9.37141681792158732e-01,
      -3.92852847300141583e-01, +9.19601348611544633e-01,
      -4.81247719908498961e-01, +8.76584640569791684e-01,
      -6.43305557445246312e-01, +7.65609534789151391e-01,
      -8.70034418599373249e-01, +4.92990984149254574e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.09025676054043075e-01, +9.51020902584829964e-01, /* pole */
      -3.08935299978518596e-01, +9.50964788334798139e-01, /* pole */
      -3.08719019796809713e-01, +9.50877808127047364e-01, /* pole */
      -3.08245730726510125e-01, +9.50707053795374568e-01, /* pole */
      -3.07228796081139122e-01, +9.50348299407557162e-01, /* pole */
      -3.05053557321103119e-01, +9.49581011399431074e-01, /* pole */
      -3.00410909845701846e-01, +9.47926566210479438e-01, /* pole */
      -2.90533068098855207e-01, +9.44321850122838535e-01, /* pole */
      -2.69654574576017103e-01, +9.36314602621993175e-01, /* pole */
      -2.26182159918911163e-01, +9.17896077942534916e-01, /* pole */
      -1.38911793452337201e-01, +8.73249112295954144e-01, /* pole */
      +2.02016274055710418e-02, +7.59973260827849550e-01, /* pole */
      +2.40913839950785386e-01, +4.86929466734396243e-01, /* pole */
      +3.66444111620866098e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.62140751442509633e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              27,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.063761861915425114,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -83.435580995768092,
    };
    static const double zeros[] = {
      -3.08730341246017170e-01, +9.51149607787396323e-01,
      -3.08642084658912130e-01, +9.51178250159979699e-01,
      -3.08413027870344136e-01, +9.51252544932125965e-01,
      -3.07906748798205232e-01, +9.51416540766723773e-01,
      -3.06821445979841123e-01, +9.51767093509141793e-01,
      -3.04508901488986516e-01, +9.52509490196276620e-01,
      -2.99582271111812959e-01, +9.54070470581438901e-01,
      -2.89062015494683244e-01, +9.57310373493440947e-01,
      -2.66477089988083948e-01, +9.63841252754561562e-01,
      -2.17490038351172005e-01, +9.76062540628419795e-01,
      -1.09480306186168080e-01, +9.93988965007853054e-01,
      +1.30604211679234056e-01, +9.91434586794129213e-01,
      +6.06094621587080717e-01, +7.95392550683757360e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -3.08983851169150781e-01, +9.51030538821015181e-01, /* pole */
      -3.09024852322393850e-01, +9.50921866269309191e-01, /* pole */
      -3.09152808509969235e-01, +9.50706348372192256e-01, /* pole */
      -3.09443678059190208e-01, +9.50255815775377988e-01, /* pole */
      -3.10069718237193315e-01, +9.49302524465628794e-01, /* pole */
      -3.11399728698241618e-01, +9.47280799781599625e-01, /* pole */
      -3.14208377743374845e-01, +9.42994811157823243e-01, /* pole */
      -3.20092113087839247e-01, +9.33926897633918340e-01, /* pole */
      -3.32222040715556555e-01, +9.14830302667475226e-01, /* pole */
      -3.56399180287399475e-01, +8.75024678769282804e-01, /* pole */
      -4.01260191222742979e-01, +7.93960261472859585e-01, /* pole */
      -4.72778703889983332e-01, +6.37462690255273023e-01, /* pole */
      -5.55025313416271482e-01, +3.68910158387076970e-01, /* pole */
      -5.94956888693383035e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.06976786939034501e-03;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_PASS,
      /* order = */              27,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.063761861915425114,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     43101.860057048936,
      /* stopband_edge = */      0,
      /* stopband_db = */        -118.85194652214399,
    };
    static const double zeros[] = {
      -9.49272349506977586e-01, +3.14455094507153143e-01,
      -3.07004424943060406e-01, +9.51708087107270950e-01,
      -9.49310446779309314e-01, +3.14340063682102488e-01,
      -3.06655289138379750e-01, +9.51820641530460199e-01,
      -9.49399514152408863e-01, +3.14070951422079025e-01,
      -3.05837725881975109e-01, +9.52083654637207677e-01,
      -9.49569583859156574e-01, +3.13556383142726325e-01,
      -3.04271470561059854e-01, +9.52585362160583538e-01,
      -9.49877754058400470e-01, +3.12621580101196561e-01,
      -3.01416002344049638e-01, +9.53492733863731012e-01,
      -9.50426652845515307e-01, +3.10948834313413136e-01,
      -2.96273766295199636e-01, +9.55103060096269108e-01,
      -9.51396356215879146e-01, +3.07969111079582081e-01,
      -2.87009184074459911e-01, +9.57927830401076097e-01,
      -9.53096167827546692e-01, +3.02667631028567197e-01,
      -2.70190203487571334e-01, +9.62806966083723736e-01,
      -9.56039371374774571e-01, +2.93238333751448144e-01,
      -2.39188898261665228e-01, +9.70973053667490094e-01,
      -9.61018441546777513e-01, +2.76484276238275706e-01,
      -1.80538823223227229e-01, +9.83567859026093005e-01,
      -9.69048481399184092e-01, +2.46870493777476802e-01,
      -6.50709940181338070e-02, +9.97880637019023409e-01,
      -9.80685987613428733e-01, +1.95588838379581437e-01,
      +1.71856407270205402e-01, +9.85122010352107336e-01,
      -9.93640754730957809e-01, +1.12596849590442555e-01,
      +6.24469928786170692e-01, +7.81048851251824661e-01,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.49015506092983130e-01, +3.15073937911496516e-01, /* pole */
      -3.08880370506975477e-01, +9.50945418116104446e-01, /* pole */
      -9.48873470630522853e-01, +3.15138023608359019e-01, /* pole */
      -3.09076640392392465e-01, +9.50518197116107855e-01, /* pole */
      -9.48634184498562183e-01, +3.15318751431367950e-01, /* pole */
      -3.09626436907459057e-01, +9.49800782738965177e-01, /* pole */
      -9.48216395613989294e-01, +3.15677256053448041e-01, /* pole */
      -3.10714016269828863e-01, +9.48551658944421949e-01, /* pole */
      -9.47478167828851303e-01, +3.16334585355540876e-01, /* pole */
      -3.12701164206059590e-01, +9.46352951937595388e-01, /* pole */
      -9.46168501883689061e-01, +3.17511886353454453e-01, /* pole */
      -3.16239475373061296e-01, +9.42477407063814332e-01, /* pole */
      -9.43841575404382849e-01, +3.19602666624823284e-01, /* pole */
      -3.22459256356864787e-01, +9.35668970250584930e-01, /* pole */
      -9.39704114801399815e-01, +3.23297696938936951e-01, /* pole */
      -3.33256294847991685e-01, +9.23801230758212633e-01, /* pole */
      -9.32344726866482398e-01, +3.29793051794885161e-01, /* pole */
      -3.51655125055043594e-01, +9.03413469568890903e-01, /* pole */
      -9.19265003681816739e-01, +3.41117757336034944e-01, /* pole */
      -3.82077169999410993e-01, +8.69295113343820036e-01, /* pole */
      -8.96138493713769746e-01, +3.60602878725728326e-01, /* pole */
      -4.30011489877347108e-01, +8.14813302124064220e-01, /* pole */
      -8.56026135367816354e-01, +3.93450083300512454e-01, /* pole */
      -5.00227797513505323e-01, +7.34739928742115800e-01, /* pole */
      -7.90385575304047183e-01, +4.47085602971915907e-01, /* pole */
      -5.92841613861716743e-01, +6.32701401800106700e-01, /* pole */
      -6.97208194281808025e-01, +5.28690523055590833e-01, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.13051707933898271e+00;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_BAND_STOP,
      /* order = */              27,
      /* sampling_frequency = */ 96000.0000,
      /* passband_ripple_db = */ 0.00081245157693902146,
      /* passband_edge = */      28800,
      /* passband_edge2 = */     43101.860057048936,
      /* stopband_edge = */      0,
      /* stopband_db = */        -118.85194652214399,
    };
    static const double zeros[] = {
      -9.48523649167602456e-01, +3.16706310277795000e-01,
      -3.13797818161585640e-01, +9.49489825810170718e-01,
      -9.48452170732848154e-01, +3.16920305174894334e-01,
      -3.14439695039696765e-01, +9.49277450581937243e-01,
      -9.48290055503667628e-01, +3.17405057667408785e-01,
      -3.15891223806525900e-01, +9.48795412468892319e-01,
      -9.47993698033860799e-01, +3.18289095773143571e-01,
      -3.18529506362816683e-01, +9.47912946201422302e-01,
      -9.47482839107454344e-01, +3.19806612809800661e-01,
      -3.23031755501795748e-01, +9.46388125949088610e-01,
      -9.46617505605327980e-01, +3.22358958432283904e-01,
      -3.30529034431724933e-01, +9.43795823998830552e-01,
      -9.45156075450787503e-01, +3.26619033490801625e-01,
      -3.42835217866718556e-01, +9.39395557467821773e-01,
      -9.42678331902676581e-01, +3.33702805743654685e-01,
      -3.62735950933675111e-01, +9.31891962568753174e-01,
      -9.38440561875005574e-01, +3.45440750097211202e-01,
      -3.94220147456424808e-01, +9.19016036497424138e-01,
      -9.31096390880223446e-01, +3.64773232145427895e-01,
      -4.42260576600431388e-01, +8.96886605087317546e-01,
      -9.18166934425126868e-01, +3.96193741152437662e-01,
      -5.11222147854053621e-01, +8.59448611345371982e-01,
      -8.95168522263082744e-01, +4.45727850542602833e-01,
      -6.00705599443307592e-01, +7.99470313893803408e-01,
      -8.54919591877047469e-01, +5.18760533796454482e-01,
      -7.00076548360994533e-01, +7.14067802407415630e-01,
      -7.89763351101144484e-01, +6.13411647474589761e-01,
    };
    static const double poles[] = {
      -9.48831995273435425e-01, +3.15295306381590923e-01, /* pole */
      -3.09551719875658438e-01, +9.50396339163792181e-01, /* pole */
      -9.48551423427769991e-01, +3.15038038779613228e-01, /* pole */
      -3.08782554565575629e-01, +9.49544921856474100e-01, /* pole */
      -9.48206829832732967e-01, +3.14552163531043150e-01, /* pole */
      -3.07327788823698933e-01, +9.48492095328605789e-01, /* pole */
      -9.47706573200742475e-01, +3.13709254664420567e-01, /* pole */
      -3.04804617696616875e-01, +9.46949872731898012e-01, /* pole */
      -9.46917451039264280e-01, +3.12287088189589279e-01, /* pole */
      -3.00552773648741833e-01, +9.44485831876365034e-01, /* pole */
      -9.45628982903493176e-01, +3.09912388507390502e-01, /* pole */
      -2.93470949111841350e-01, +9.40382799369885447e-01, /* pole */
      -9.43496784634606400e-01, +3.05966418469117429e-01, /* pole */
      -2.81759131511626293e-01, +9.33379420082324884e-01, /* pole */
      -9.39950095291049337e-01, +2.99433049999425271e-01, /* pole */
      -2.62546916365482608e-01, +9.21151561384922712e-01, /* pole */
      -9.34041909488610322e-01, +2.88660031093888825e-01, /* pole */
      -2.31467557738280316e-01, +8.99224302736274961e-01, /* pole */
      -9.24222216903470506e-01, +2.70991332646744021e-01, /* pole */
      -1.82588599706939775e-01, +8.58675878913464530e-01, /* pole */
      -9.08082349619884766e-01, +2.42189027138394020e-01, /* pole */
      -1.10311320470479438e-01, +7.81705204507688989e-01, /* pole */
      -8.82542078180880263e-01, +1.95367462826336463e-01, /* pole */
      -1.75756641107406714e-02, +6.35825920207473860e-01, /* pole */
      -8.47357636953078086e-01, +1.18687268265209214e-01, /* pole */
      +6.70549949620148078e-02, +3.77802886134051386e-01, /* pole */
      +9.82015859705722000e-02, +0.00000000000000000e+00, /* pole */
      -8.24054551084538733e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +1.40680409573448982e-02;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_LOW_PASS,
      /* order = */              7,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0.00051887705966349696,
      /* passband_edge = */      230399.99999999997,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -116.4238243555032,
    };
    static const double zeros[] = {
      -9.97027855227194060e-01, +7.70419100299392368e-02,
      -9.98066800887137173e-01, +6.21503094675779771e-02,
      -9.99396076249871701e-01, +3.47488528783430062e-02,
      -1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -9.37402909062819090e-01, +2.37321749775770696e-01, /* pole */
      -8.59187181097440855e-01, +2.20495014261374411e-01, /* pole */
      -7.51410598918326511e-01, +1.60502392243735331e-01, /* pole */
      -6.81745326679053520e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +5.44221321322100993e-01;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
  {
    static const BseIIRFilterRequest filter_request = {
      /* kind = */               BSE_IIR_FILTER_ELLIPTIC,
      /* type = */               BSE_IIR_FILTER_HIGH_PASS,
      /* order = */              7,
      /* sampling_frequency = */ 512000.0000,
      /* passband_ripple_db = */ 0.00072885510688855426,
      /* passband_edge = */      230399.99999999997,
      /* passband_edge2 = */     0,
      /* stopband_edge = */      0,
      /* stopband_db = */        -116.4238243555032,
    };
    static const double zeros[] = {
      -4.25112941537199807e-01, +9.05140313397646334e-01,
      -2.34427398926797148e-01, +9.72133630028514273e-01,
      +3.30035630110736999e-01, +9.43968475563357390e-01,
      +1.00000000000000000e+00, +0.00000000000000000e+00,
    };
    static const double poles[] = {
      -8.81533739321557097e-01, +3.57120725937826067e-01, /* pole */
      -8.20314318178990032e-01, +2.69567610487590481e-01, /* pole */
      -7.85259801353166420e-01, +1.45305438652743063e-01, /* pole */
      -7.73732041862753905e-01, +0.00000000000000000e+00, /* pole */
    };
    filters[index].filter_request = &filter_request;
    filters[index].gain = +2.41313643240746080e-05;
    filters[index].n_zeros = RAPICORN_ARRAY_SIZE (zeros) / 2;
    filters[index].zeros = zeros;
    filters[index].n_poles = RAPICORN_ARRAY_SIZE (poles) / 2;
    filters[index].poles = poles;
    index++;
  }
