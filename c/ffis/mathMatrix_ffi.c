#include <lean/lean.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../utils/utils.h"
#include <cblas.h>
#include <stdio.h>


typedef struct mathMatrix {
    double*   data;
    uint32_t  rows;
    uint32_t  cols;
} mathMatrix;

//good
static lean_external_class* g_mathMatrix_external_class = NULL;

extern lean_object* l_instInhabitedFloat;

//good
void mathMatrix_finalizer(void* M_) {
    mathMatrix* v = (mathMatrix*) M_;
    if (v->data) {
        free(v->data);
    }
    free(v);
}
//good
lean_obj_res mathMatrix_initialize() {
    g_mathMatrix_external_class = lean_register_external_class(
        mathMatrix_finalizer,
        noop_foreach
    );
    return lean_io_result_mk_ok(lean_box(0));
}

//functions for interacting with the struct

//good
mathMatrix* mathMatrix_alloc(uint32_t rows, uint32_t columns){
    mathMatrix* out = (mathMatrix*) malloc(sizeof(mathMatrix));
    if (out == NULL) {
        printf("Insufficient Memory\n");
    }

    out->data = (double*)(malloc(sizeof(double)*rows*columns));
    if (out->data == NULL) {
        printf("Insufficient Memory\n");
    }
    out->rows = rows;
    out->cols = columns;
    return out;
}


double mathMatrix_struct_get(mathMatrix* M, uint32_t i, uint32_t j) {
    assert(i < M->rows);
    assert(j < M->cols);

    return M->data[(i*M->cols) + j];
}

void mathMatrix_struct_set(mathMatrix* M, uint32_t i, uint32_t j, double val) {
    M->data[(i*M->cols)+j] = val;
}

mathMatrix* mathMatrix_copy(mathMatrix* M) {
    mathMatrix* out = mathMatrix_alloc(M->rows, M->cols);
    for (size_t i = 0; i < M->rows; i++) {
        for (size_t j = 0; j < M->cols; j++) {
            mathMatrix_struct_set(out, i, j, mathMatrix_struct_get(M, i, j));
        }
    }

    return out;
}

// Not sure if I even need this
// :soFalse:
lean_object* mathMatrix_boxer(mathMatrix* M) {
    return lean_alloc_external(g_mathMatrix_external_class, M);
}
//good
mathMatrix* mathMatrix_unboxer(lean_object* o) {
    return (mathMatrix*) lean_get_external_data(o);
}

lean_object* mathMatrix_new(lean_object* rows_, lean_object* cols_, double val) {
    uint32_t rows = lean_unbox_uint32(rows_);
    uint32_t cols = lean_unbox_uint32(cols_);

    if (rows == 0 || cols == 0) {
        return make_error("invalid rows/cols");
    }

    mathMatrix* m = mathMatrix_alloc(rows, cols);

    if (!m) {
        return make_error("ERROR_INSUF_MEM");
    }

    for (size_t i = 0; i< rows*cols; i++) {
        m->data[i] = val;
    }

    lean_object* out = mathMatrix_boxer(m);
    return out;
}

uint32_t mathMatrix_isEqv(lean_object* m, lean_object* n, lean_object* M1_, lean_object* M2_) {
    mathMatrix* M1 = mathMatrix_unboxer(M1_);
    mathMatrix* M2 = mathMatrix_unboxer(M2_);

    assert(M1->rows == M2->rows);
    assert(M1->cols == M2->cols);

    for (size_t i = 0; i < (M1->rows*M1->cols); i++) {
        if (M1->data[i] != M2->data[i])
            return 0;
    }
    return 1;
}
//good
double mathMatrix_get_val(lean_object* rows_, lean_object* cols_, lean_object* M_, lean_object* i_, lean_object* j_) {
    mathMatrix* M = mathMatrix_unboxer(M_);

    return mathMatrix_struct_get(M, lean_unbox_uint32(i_), lean_unbox_uint32(j_));
}

lean_object* mathMatrix_set_val(lean_object* rows_, lean_object* cols_, lean_object* M_, lean_object* i_, lean_object* j_, double x) {
    mathMatrix* M = mathMatrix_unboxer(M_);

    mathMatrix* out_struct = mathMatrix_copy(M);
    
    uint32_t idx = lean_unbox_uint32(i_);
    uint32_t jdx = lean_unbox_uint32(j_);
    mathMatrix_struct_set(out_struct, idx, jdx, x);

    return mathMatrix_boxer(out_struct);
}

lean_object* mathMatrix_transpose(lean_object* rows_, lean_object* cols_, lean_object* M_) {
    mathMatrix* M = mathMatrix_unboxer(M_);
    mathMatrix* out_struct = mathMatrix_alloc(M->cols, M->rows);
    for (size_t i = 0; i < out_struct->rows; i++) {
        for (size_t j = 0; j < out_struct->cols; j++) {
            mathMatrix_struct_set(out_struct, i, j, mathMatrix_struct_get(M, j, i));
        }
    }

    return mathMatrix_boxer(out_struct);
}

lean_object* mathMatrix_mul(lean_object* m_, lean_object* n_, lean_object* k_, lean_object* M1_, lean_object* M2_) {
    //uint32_t m = lean_unbox_uint32(m_);
    //uint32_t n = lean_unbox_uint32(n_);
    //uint32_t k = lean_unbox_uint32(k_);
    mathMatrix* M1 = mathMatrix_unboxer(M1_);
    mathMatrix* M2 = mathMatrix_unboxer(M2_);
    uint32_t m = M1->rows;
    uint32_t n = M1->cols;
    uint32_t k = M2->cols;

    assert(n = M2->rows);

    mathMatrix* out_struct = mathMatrix_alloc(M1->rows, M2->cols);
    
    double* result = malloc(sizeof(double)*m*k);
    for (size_t i = 0; i < m*k; i++) {
        result[i] = 0.0;
    }

    /*
    for (int i = 0; i < m*n; i++) {
        printf("%f\n",M1->data[i]);
    }
    
    for (int i = 0; i < k*n; i++) {
        printf("%f\n",M2->data[i]);
    }
    */

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                m, k, n, 1.0, M1->data, n, M2->data, k, 0.0, result, k);

    out_struct->data = result;
    /*
    for (size_t i = 0; i < m*k; i++) {
        printf("%f\n", result[i]);
    }
    */

    return mathMatrix_boxer(out_struct);

}
