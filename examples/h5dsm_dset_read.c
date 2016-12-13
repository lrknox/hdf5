#include "h5dsm_example.h"
#include <time.h>

int main(int argc, char *argv[]) {
    uuid_t pool_uuid;
    char *pool_grp = "daos_tier0";
    hid_t file = -1, dset = -1, trans = -1, fapl = -1;
    int buf[4][6];
    int i, j;
    uint64_t trans_num;

    (void)MPI_Init(&argc, &argv);
    (void)daos_init();

    /* Seed random number generator */
    srand(time(NULL));

    if(argc < 4 || argc > 5)
        PRINTF_ERROR("argc must be 4 or 5\n");

    /* Parse UUID */
    if(0 != uuid_parse(argv[1], pool_uuid))
        ERROR;

    /* Set up FAPL */
    if((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        ERROR;
    if(H5Pset_fapl_daosm(fapl, MPI_COMM_WORLD, MPI_INFO_NULL, pool_uuid, pool_grp) < 0)
        ERROR;

    /* Open file */
    if((file = H5Fopen_ff(argv[2], H5F_ACC_RDONLY, fapl, argc == 4 ? &trans : NULL)) < 0)
        ERROR;

    /* Create transaction if specified */
    if(argc == 5) {
        trans_num = (uint64_t)atoi(argv[4]);
        if((trans = H5TRcreate(file, trans_num)) < 0)
            ERROR;
    }
    else
        if(H5TRget_trans_num(trans, &trans_num) < 0)
            ERROR;

    /* Open dataset */
    if((dset = H5Dopen_ff(file, argv[3], H5P_DEFAULT, trans)) < 0)
        ERROR;

    printf("Reading dataset - transaction number = %llu\n", (long long unsigned)trans_num);

    /* Initialize buffer */
    for(i = 0; i < 4; i++)
        for(j = 0; j < 6; j++)
            buf[i][j] = -1;

    /* Read data */
    if(H5Dread_ff(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf, trans) < 0)
        ERROR;

    /* Print buffer */
    printf("Successfully read data. Buffer is:\n");
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 6; j++)
            printf("%d ", buf[i][j]);
        printf("\n");
    }

    /* Close */
    if(H5Dclose_ff(dset, -1) < 0)
        ERROR;
    if(H5TRclose(trans) < 0)
        ERROR;
    if(H5Fclose_ff(file, -1) < 0)
        ERROR;
    if(H5Pclose(fapl) < 0)
        ERROR;

    printf("Success\n");

    (void)daos_fini();
    (void)MPI_Finalize();
    return 0;

error:
    H5E_BEGIN_TRY {
        H5Dclose_ff(dset, -1);
        H5TRclose(trans);
        H5Fclose_ff(file, -1);
        H5Pclose(fapl);
    } H5E_END_TRY;

    (void)daos_fini();
    (void)MPI_Finalize();
    return 1;
}
