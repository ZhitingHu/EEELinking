load('output.txt')
gnd = output(:,1);
pred = output(:,2);
[~, gnd_idx] = sort(gnd);
[~, pred_idx] = sort(-1*pred);
gnd_sort = zeros(size(gnd));
gnd_sort(gnd_idx) = 1:length(gnd_idx);
pred_sort = zeros(size(pred));
pred_sort(pred_idx) = 1:length(pred_idx);

d = gnd_sort - pred_sort;
sum_temp = sum(d.*d);
score_cor = 1 - 6 * sum_temp / length(gnd) / (length(gnd)^2 - 1);
score_cor
