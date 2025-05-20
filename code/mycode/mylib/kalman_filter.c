
void mat_multiply(float A[4][4], float B[4][4], float out[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                out[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void mat_add(float A[4][4], float B[4][4], float out[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out[i][j] = A[i][j] + B[i][j];
        }
    }
}

void kalman_filter(float x_hat[4], float cov[4][4], float x_var, float y_var, float co_var, float pos[2]) {
    // Init
    float dt = 0.5;
    float C[4][4] = {
        {1, 0, dt, 0},
        {0, 1, 0, dt},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    float H[2][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0}
    };

    float proc_err = 0.1;

    float Q[4][4] = {
        {proc_err, 0, 0, 0},
        {0, proc_err, 0, 0},
        {0, 0, proc_err, 0},
        {0, 0, 0, proc_err}
    };

    float R[2][2] = {
        {x_var * x_var, co_var},
        {co_var, y_var * y_var}
    };

    // prediction
    float x_est[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            x_est[i] += C[i][j] * x_hat[j];
        }
    }

    float cov_est[4][4], cov_est1[4][4];
    for (int i = 0; i < 4; i++) { // init cov_est
        for (int j = 0; j < 4; j++) {
            cov_est[i][j] = 0;
            cov_est1[i][j] = 0;
        }
    }

    // self.cov_est = self.A @ self.cov @ self.A.T + self.Q_k
    // C * cov
    mat_multiply(C, cov, cov_est1);
    // C_T
    float C_T[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            C_T[i][j] = C[j][i];
        }
    }

    // (C * cov) * C_T
    mat_multiply(cov_est1, C_T, cov_est);

    // ((C * cov) * C_T) + Q
    mat_add(cov_est, Q, cov_est);

    // update
    // error_x = obs - self.H @ self.x_hat_est
    // obs is current measurement [x y]
    float err_x[2] = {0, 0};
    // H * x_est
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            err_x[i] += H[i][j] * x_est[j];
        }
    }
    // pos - (H * x_est)
    for (int i = 0; i < 2; i++) {
        err_x[i] = pos[i] - err_x[i];
    }

    // error_cov = self.H @ self.cov_est @ self.H.T + self.R
    float err_cov1[2][4];

    // H * cov_est
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 4; i++) {
            err_cov1[k][i] = 0;
            for (int j = 0; j < 4; j++) {
                err_cov1[k][i] += H[k][j] * cov_est[j][i];
            }
        }
    }

    // H_t
    float H_t[4][2];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            H_t[i][j] = H[j][i];
        }
    }

    // (H * cov_est) * H_T
    float err_cov[2][2] = {{0, 0}, {0, 0}};
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                err_cov[k][i] += err_cov1[k][j] * H_t[j][i];
            }
        }
    }

    // ((H * cov_est) * H_T) + R
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            err_cov[i][j] += R[i][j];
        }
    }

    // inverse of error_cov
    float err_cov_i[2][2] = {{err_cov[1][1], -1 * err_cov[0][1]},
        {-1 * err_cov[1][0], err_cov[0][0]}};
    float det = (err_cov[0][0] * err_cov[1][1]) - (err_cov[0][1] * err_cov[1][0]);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            err_cov_i[i][j] *= 1 / det;
        }
    }

    // K = self.cov_est @ self.H.T @ np.linalg.inv(error_cov)
    float CH[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                CH[k][i] += cov_est[k][j] * H_t[j][i];
            }
        }
    }

    float K[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                K[k][i] += CH[k][j] * err_cov_i[j][i];
            }
        }
    }

    // self.x_hat = self.x_hat_est + K @ error_x
    for (int k = 0; k < 4; k++) {
        x_hat[k] = 0;
        for (int i = 0; i < 2; i++) {
            x_hat[k] += K[k][i] * err_x[i]; 
        }
    }
    for (int k = 0; k < 4; k++) {
        x_hat[k] = x_est[k] + x_hat[k];
    }

    // self.cov = (np.eye(self.ndim) - K @ self.H) @ self.cov_est
    float KH[4][4] = {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    };
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) {
                KH[k][i] += K[k][j] * H[j][i];
            }
        }
    }
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 4; i++) {
            if (k == i) {
                KH[k][i] = 1 - KH[k][i];
            } else {
                KH[k][i] = 0 - KH[k][i];
            }
        }
    }
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 4; i++) {
            cov[k][i] = 0;
            for (int j = 0; j < 4; j++) {
                cov[k][i] += KH[k][j] * cov_est[j][i];
            }
        }
    }
}