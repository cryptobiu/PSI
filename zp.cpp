//
// Created by bush on 29/01/18.
//

#include "zp.h"
#include <vector>
#include <thread>
#include <NTL/BasicThreadPool.h>
#include <omp.h>


#define LEFT(X) (2*X+1)
#define RIGHT(X) (2*X+2)
#define PAPA(X) ((X-1)/2)




void print_poly (ZZ_pX& P)
{
    long degree = deg(P);
    if (-1 == degree) {
        cout << "0";
        return;
    }
    for(long i=0; i<=degree; i++) {
        cout << coeff(P, i);
        if (i==1)
            cout << "X";
        else if(i>1)
            cout << "X^" << i;
        if (i<degree) {
            cout << " + ";
        }
    }
//    cout << endl << "random poly:" << endl << P << endl;
}

void build_tree_main(ZZ_pX* tree, ZZ_p* points, unsigned int tree_size) {

    ZZ_p negated;
    int i;// = tree_size-1;
    unsigned int point_index;


    steady_clock::time_point begin1 = steady_clock::now();


    for (i = tree_size-1; i>=tree_size/2; i--) {
        point_index = i-(tree_size-1)/2;
        NTL::negate(negated, points[point_index]);
        SetCoeff(tree[i], 0, negated);
        SetCoeff(tree[i], 1, 1);
    }
    steady_clock::time_point end1 = steady_clock::now();
    cout << "Building tree - first part: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

    for (; i>=0; i--) {
        tree[i] = tree[LEFT(i)]*tree[RIGHT(i)];
    }


}

void build_tree(ZZ_pX* tree, ZZ_p* points, unsigned int tree_size) {

    ZZ_p negated;
    int i;// = tree_size-1;
    unsigned int point_index;


    steady_clock::time_point begin1 = steady_clock::now();

    //cout<<"leaves";
    for (i = tree_size-1; i>=tree_size/2; i--) {
        point_index = i-(tree_size-1)/2;
        NTL::negate(negated, points[point_index]);
        SetCoeff(tree[i], 0, negated);
        SetCoeff(tree[i], 1, 1);

        //cout<<"--"<<i;
    }
    steady_clock::time_point end1 = steady_clock::now();
    cout << "Building tree - first part: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

    begin1 = steady_clock::now();
    int numThreads = 4;
    vector<vector<int>> subs(numThreads);
    generateSubTreeArrays(subs, tree_size/2, numThreads-1);
    end1 = steady_clock::now();
    cout << "Building tree - generate sub trees: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    vector<thread> threads(numThreads);

//    SetNumThreads(numThreads);
//
//    NTL_EXEC_RANGE(numThreads, first, last)
//    for (int t=first; t<last; t++) {
//
//        buildSubTree(tree, ref(subs[t]));
//        //threads[t] = thread(&buildSubTree, tree, ref(subs[i]));
//
//
//    }
//
//    NTL_EXEC_RANGE_END


    for (int t=0; t<numThreads; t++) {

        threads[t] = thread(&buildSubTree, tree, ref(subs[t]));
    }

    for (int t=0; t<numThreads; t++){
        threads[t].join();
    }

    end1 = steady_clock::now();
    cout << "Building tree - threads part: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    vector<thread> threadsLastPart(numThreads*2);

    for(int i=log2(numThreads)-1; i>=1; i--){
        //cout<<"layer "<<i<< " : " <<endl;
        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index to process is "<< j<<endl;

            threadsLastPart[j] = thread(&buildTreeSpecific,tree, j);
        }

        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index join to process is "<< j<<endl;
            threadsLastPart[j].join();
        }
    }


    end1 = steady_clock::now();
    cout << "inter - last part for max "<< numThreads/2<<" threads " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;



//    begin1 = steady_clock::now();
//    for(int i=numThreads-2; i>=1; i--)
//        tree[i] = tree[LEFT(i)]*tree[RIGHT(i)];
//    end1 = steady_clock::now();
//    cout << "Building tree - SERIAL: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

    begin1 = steady_clock::now();
    tree[0] = tree[LEFT(0)]*tree[RIGHT(0)];
    end1 = steady_clock::now();
    cout << "Building tree - last mults: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

}



void generateSubTreeArrays (vector<vector<int>> &subArrays, int totalNodes, int firstIndex) {

    int numOfSubTrees = subArrays.size();

    //generate subtree array
    int maxElement = totalNodes;

    for(int i=0; i<numOfSubTrees;i++){
        subArrays[i].resize(maxElement);//this is overkill, but will be reduces later
        subArrays[i][0] = firstIndex+i;
    }

    for(int i=0; i<numOfSubTrees;i++) {
        for (int j = 1; j < totalNodes; j++) {

            if (j % 2 == 1)
                subArrays[i][j] = LEFT(subArrays[i][PAPA(j)]);
            else
                subArrays[i][j] = RIGHT(subArrays[i][PAPA(j)]);


            if (subArrays[i][j] >= maxElement) {
                subArrays[i].resize(j);
                //cout << "j is in first loop " << j << endl;
                break;

            }

            //cout << subArrays[i][j] << "-";
        }
        //cout << "size of "<<i<<" array" << subArrays[i].size() << endl;
        //cout << "-------------------" << endl;

    }

}

void buildSubTree (ZZ_pX* tree, vector<int> &subTree) {


    //cout<<"Thread indices";
    ZZ_p::init(ZZ(2305843009213693951));
    int index;
    for (int i=subTree.size()-1; i>=0; i--) {
        index = subTree[i];
        //cout<<"--"<<index;

        tree[index] = tree[LEFT(index)]*tree[RIGHT(index)];
    }

}

void interSubTree (ZZ_pX* temp, ZZ_pX* M,  vector<int> &subTree) {


    //cout<<"Thread indices";
    ZZ_p::init(ZZ(2305843009213693951));
    int index;
    for (int i=subTree.size()-1; i>=0; i--) {
        index = subTree[i];
        //cout<<"--"<<index;


        temp[index] = temp[LEFT(index)] * M[RIGHT(index)] + temp[RIGHT(index)] * M[LEFT(index)] ;

    }

}

void evalSubTree (ZZ_pX* reminders, ZZ_pX* tree,  vector<int> &subTree) {


    //cout<<"Thread indices";
    ZZ_p::init(ZZ(2305843009213693951));
    int index;

    unsigned int i = 1;
    for (i=0; i<subTree.size(); i++) {
        index = subTree[i];
//        cout << "i="<<i <<": ";
        reminders[index] = reminders[PAPA(index)]%tree[index];
    }

}

void test_tree (ZZ_pX& final_polynomial, ZZ_p* points, unsigned int npoints) {
    ZZ_p result;
    bool error = false;
    for (unsigned int i=0; i<npoints; i++) {
        result = eval(final_polynomial, points[i]);
        if (0!=result) {
            cout << "FATAL ERROR: polynomials tree is incorrect!" << endl;
            error = true;
            break;
        }
    }
    if (!error)
        cout << "polynomials tree is correct." << endl;
}

void evaluate_main (ZZ_pX& P, ZZ_pX* tree, ZZ_pX* reminders , unsigned int tree_size, ZZ_p* results) {

    reminders[0] = P%tree[0];

    unsigned int i = 1;
    for (; i<tree_size/2; i++) {
        reminders[i] = reminders[PAPA(i)]%tree[i];
    }

    unsigned int result_index;
    for (; i<tree_size; i++) {
        reminders[i] = reminders[PAPA(i)]%tree[i];
        result_index = i-(tree_size-1)/2;
        results[result_index] = coeff(reminders[i], 0);
    }
}


void evaluate (ZZ_pX& P, ZZ_pX* tree, ZZ_pX* reminders , unsigned int tree_size, ZZ_p* results) {

    auto begin1 = steady_clock::now();
    //set the reminder of the root
    reminders[0] = P%tree[0];
    auto end1 = steady_clock::now();
    cout << "eval - not paralleled "<<" threads " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;



    begin1 = steady_clock::now();
    int numThreads = 4;
    vector<vector<int>> subs(numThreads);
    generateSubTreeArrays(subs, tree_size, numThreads-1);
    end1 = steady_clock::now();
    cout << "eval - generate sub trees: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    vector<thread> threadsFirstPart(numThreads*2);

    for(int i=1; i<log2(numThreads); i++){
        //cout<<"layer "<<i<< " : " <<endl;
        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index to process is "<< j<<endl;

            threadsFirstPart[j] = thread(&evalReminder,tree, reminders, j);
        }

        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index join to process is "<< j<<endl;
            threadsFirstPart[j].join();
        }
    }


    end1 = steady_clock::now();
    cout << "eval - first part for max "<< numThreads/2<<" threads " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    vector<thread> threads(numThreads);

    for (int t=0; t<numThreads; t++) {

        threads[t] = thread(&evalSubTree, reminders, tree, ref(subs[t]));


    }


    for (int t=0; t<numThreads; t++){
        threads[t].join();
    }

    end1 = steady_clock::now();
    cout << "eval - thread part for "<< numThreads<<" threads " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

    begin1 = steady_clock::now();
    unsigned int result_index;
    for (int i=tree_size/2; i<tree_size; i++) {
        result_index = i-(tree_size-1)/2;
        results[result_index] = coeff(reminders[i], 0);
    }

    end1 = steady_clock::now();
    cout << "eval - assign last part: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

}

void evalReminder(ZZ_pX *tree, ZZ_pX *reminders, int i) {
    ZZ_p::init(ZZ(2305843009213693951));
    reminders[i] = reminders[PAPA(i)]%tree[i];
}

void interSpecific(ZZ_pX *temp, ZZ_pX *M, int i) {
    ZZ_p::init(ZZ(2305843009213693951));
    temp[i] = temp[LEFT(i)] * M[RIGHT(i)] + temp[RIGHT(i)] * M[LEFT(i)] ;
}

void buildTreeSpecific(ZZ_pX *tree, int i) {
    ZZ_p::init(ZZ(2305843009213693951));
    tree[i] = tree[LEFT(i)]*tree[RIGHT(i)];
}


void test_evaluate(ZZ_pX& P, ZZ_p* points, ZZ_p* results, unsigned int npoints) {
    bool error = false;
    for (unsigned int i = 0; i < npoints; i++) {
        ZZ_p y = eval(P, points[i]);
        if (y != results[i]) {
            cout << "y=" << y << " and results[i]=" << results[i] << endl;
            error = true;
        }
    }
    if (error)
        cout << "ERROR: evaluation results do not match real evaluation!" << endl;
    else
        cout << "All evaluation results computed correctly!" << endl;
}


void multipoint_evaluate_zp(ZZ_pX& P, ZZ_p* x, ZZ_p* y, long degree)
{
//    cout << "P:" <<endl; print_poly(P); cout << endl;
    // we want to evaluate P on 'degree+1' values.
    ZZ_pX* p_tree = new ZZ_pX[degree*2+1];
    steady_clock::time_point begin1 = steady_clock::now();
    build_tree (p_tree, x, degree*2+1);
    steady_clock::time_point end1 = steady_clock::now();
    //test_tree(p_tree[0], x, degree+1);

    ZZ_pX* reminders = new ZZ_pX[degree*2+1];
    steady_clock::time_point begin2 = steady_clock::now();
    evaluate(P, p_tree, reminders, degree*2+1, y);
    chrono::steady_clock::time_point end2 = steady_clock::now();


    cout << "Building tree: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;
    cout << "Evaluating points: " << duration_cast<milliseconds>(end2 - begin2).count() << " ms" << endl;
    cout << "Total: " << duration_cast<milliseconds>(end1 - begin1).count()+ duration_cast<milliseconds>(end2 - begin2).count() << " ms" << endl;


    //test_evaluate(P,x,y,10);
}


void test_multipoint_eval_zp(ZZ prime, long degree)
{
    // init underlying prime field
    ZZ_p::init(ZZ(prime));

    // the given polynomial
    ZZ_pX P;
    random(P, degree+1);
    SetCoeff(P,degree,random_ZZ_p());

    // evaluation points:
    ZZ_p* x = new ZZ_p[degree+1];
    ZZ_p* y = new ZZ_p[degree+1];

    for(unsigned int i=0;i<=degree; i++) {
        random(x[i]);
    }

    multipoint_evaluate_zp(P, x, y, degree);
}




/*
 * expects an "empty" polynomial 'resultP'
 */
void iterative_interpolate_zp_main(ZZ_pX& resultP, ZZ_pX* temp, ZZ_p* y, ZZ_p* a, ZZ_pX* M, unsigned int tree_size)
{
    int i = tree_size-1;
    ZZ_p inv_a;
    unsigned int y_index;
    for (; i>=tree_size/2; i--) {
        y_index = i-(tree_size-1)/2;
        inv(inv_a,a[y_index]); // inv_a = 1/a[y_index]
        SetCoeff(temp[i], 0, y[y_index]*inv_a);
    }

    for (; i>=0; i--) {
        temp[i] = temp[LEFT(i)] * M[RIGHT(i)] + temp[RIGHT(i)] * M[LEFT(i)] ;
    }

    resultP = temp[LEFT(0)] * M[RIGHT(0)] + temp[RIGHT(0)] * M[LEFT(0)] ;
}

void iterative_interpolate_zp(ZZ_pX& resultP, ZZ_pX* temp, ZZ_p* y, ZZ_p* a, ZZ_pX* M, unsigned int tree_size)
{
    unsigned int i = tree_size-1;
    ZZ_p inv_a;
    unsigned int y_index;
    for (; i>=tree_size/2; i--) {
        y_index = i-(tree_size-1)/2;
        inv(inv_a,a[y_index]); // inv_a = 1/a[y_index]
        SetCoeff(temp[i], 0, y[y_index]*inv_a);
    }

    auto begin1 = steady_clock::now();
    int numThreads = 4;
    vector<vector<int>> subs(numThreads);
    generateSubTreeArrays(subs, tree_size/2, numThreads-1);
    auto end1 = steady_clock::now();
    cout << "inter - generate sub trees: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    vector<thread> threads(numThreads);

    for (int t=0; t<numThreads; t++) {

        threads[t] = thread(&interSubTree, temp, M, ref(subs[t]));


    }


    for (int t=0; t<numThreads; t++){
        threads[t].join();
    }

    end1 = steady_clock::now();
    cout << "interpolate - threads part: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


//    begin1 = steady_clock::now();
//    for(int i=numThreads-2; i>0; i--)
//        temp[i] = temp[LEFT(i)] * M[RIGHT(i)] + temp[RIGHT(i)] * M[LEFT(i)] ;
//
//    end1 = steady_clock::now();
//    cout << "inter - first part SERIAL "<< duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;
//


    begin1 = steady_clock::now();
    vector<thread> threadsLastPart(numThreads*2);

    for(int i=log2(numThreads)-1; i>=1; i--){
        //cout<<"layer "<<i<< " : " <<endl;
        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index to process is "<< j<<endl;

            threadsLastPart[j] = thread(&interSpecific,temp, M, j);
        }

        for(int j=pow(2,i)-1; j<=pow(2,i+1)-2; j++){
            //cout<<" index join to process is "<< j<<endl;
            threadsLastPart[j].join();
        }
    }


    end1 = steady_clock::now();
    cout << "inter - last part for max "<< numThreads/2<<" threads " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;


    begin1 = steady_clock::now();
    //last iteration
    resultP = temp[LEFT(0)] * M[RIGHT(0)] + temp[RIGHT(0)] * M[LEFT(0)] ;

    end1 = steady_clock::now();
    cout << "interpolate - last iteration for root: " << duration_cast<milliseconds>(end1 - begin1).count() << " ms" << endl;

}


void interpolate_zp(ZZ_pX& resultP, ZZ_p* x, ZZ_p* y, long degree)
{
    system_clock::time_point begin[4];
    system_clock::time_point end[4];
    ZZ_pX *M = new ZZ_pX[degree * 2 + 1];;
    ZZ_p *a = new ZZ_p[degree + 1];;

    prepareForInterpolate(x, degree, M, a);

    //now we can apply the formula
    ZZ_pX* temp = new ZZ_pX[degree*2+1];
    begin[4] = system_clock::now();
    iterative_interpolate_zp(resultP, temp, y, a, M, degree*2+1);
    end[4] = system_clock::now();


    cout << "Interpolation: " << duration_cast<milliseconds>(end[4] - begin[4]).count() << " ms" << endl;
    cout << "Total: " << duration_cast<milliseconds>(end[1]-begin[1] + end[2]-begin[2] + end[3]-begin[3] + end[4]-begin[4]).count() << " ms" << endl;
}

void prepareForInterpolate(ZZ_p *x, long degree, ZZ_pX *M, ZZ_p *a) {

    system_clock::time_point begin[4];
    system_clock::time_point end[4];

    begin[1] = system_clock::now();
    //we first build the tree of the super modulibegin[1]= system_clock::now();
    build_tree(M,x, degree*2+1);
    end[1] = system_clock::now();
//    test_tree(M[0], x, degree+1);

    ZZ_pX D;
    //we construct a preconditioned global structure for the a_k for all 1<=k<=(degree+1)ZZ_pX D;
    begin[2] = system_clock::now();
    diff(D, M[0]);
    end[2] = system_clock::now();

    //evaluate d(x) to obtain the results in the array a
    ZZ_pX* reminders = new ZZ_pX[degree*2+1];
    begin[3] = system_clock::now();
    evaluate(D, M, reminders, degree*2+1, a);
    end[3] = system_clock::now();
//    test_evaluate(D,x,a,degree+1);

    cout << "Building tree: " << duration_cast<milliseconds>(end[1] - begin[1]).count() << " ms" << endl;
    cout << "Differentiate: " << duration_cast<milliseconds>(end[2] - begin[2]).count() << " ms" << endl;
    cout << "Evaluate diff: " << duration_cast<milliseconds>(end[3] - begin[3]).count() << " ms" << endl;

}


void test_interpolation_result_zp(ZZ_pX& P, ZZ_p* x, ZZ_p* y, long degree)
{
    cout << "Testing result polynomial" << endl;
    ZZ_p res;
    for (long i=0; i< degree+1; i++) {
        eval(res, P, x[i]);
        if (res != y[i]) {
            cout << "Error! x = " << x[i] << ", y = " << y[i] << ", res = " << res << endl;
            return;
        }
    }
    cout << "Polynomial is interpolated correctly!" << endl;
}

void test_interpolate_zp(ZZ prime, long degree)
{
    // init underlying prime field
    ZZ_p::init(ZZ(prime));

    // interpolation points:
    ZZ_p* x = new ZZ_p[degree+1];
    ZZ_p* y = new ZZ_p[degree+1];
    for(unsigned int i=0;i<=degree; i++) {
        random(x[i]);
        random(y[i]);
    }

    ZZ_pX P;
    interpolate_zp(P, x, y, degree);
    //cout << "P: "; print_poly(P); cout << endl;
    //test_interpolation_result_zp(P, x, y, degree);
}


void BytesToZZ_px(unsigned char *bytesArr, ZZ_pX& poly, long numOfElements, long sizeOfElement){

    //turn each byte to zz_p element in a vector

    vec_ZZ repFromBytes;
    repFromBytes.SetLength(numOfElements);

    for(int i=0; i<numOfElements; i++){

        ZZ zz;

        //translate the bytes into a ZZ element
        ZZFromBytes(zz, bytesArr + i*sizeOfElement, sizeOfElement);

        repFromBytes[i] = zz;
    }


    //turn the vec_zzp to the polynomial

    poly = to_ZZ_pX(to_vec_ZZ_p(repFromBytes));


}
void ZZ_pxToBytes(ZZ_pX& poly, unsigned char *bytesArr, long numOfElements, long sizeOfElement){

    //get the zz_p vector

    for(int i=0; i<numOfElements; i++){

        BytesFromZZ(bytesArr + i*sizeOfElement ,rep(poly.rep[i]),sizeOfElement);
    }


}