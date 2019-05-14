#include <iostream>
#include <stdio.h>
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <iomanip>
#include <string.h>

using namespace std;

bool nonexclusive_cache = true;

struct block
{
	long int Address, Tag; 
	string Flag;
	long int Sequence_num;
};

typedef struct block Block;

Block** init (int Sets, int Assoc)
{

	Block**  block = new Block *[Sets];

	long int size=Assoc;
	for(long int i=0; i<Sets; i++)
	{
		block[i] = new Block [Assoc];
	}
	for (int i=0; i<Sets; i++)
	{
		for(int j=0; j<Assoc; j++)
		{
			block[i][j].Sequence_num = 0;
			block[i][j].Tag = 0;
			block[i][j].Flag = "Invalid";
			block[i][j].Address = -1;
		}
	}
	return block;
}

class Cache
{
	public:
		long int Blocksize, Cache_Size;
		int Way;
		long int Assoc, Sets;
		long int Replace_Num;
		long int Index, Tag, Offset;
		int Index_Size,Index_Size_i, Tag_Size,Tag_Size_i, Offset_Size,Offset_Size_i;
		long int index_mask, tag_mask, offset_mask;
		long int EvictedAddress;
		string REPLACEMENT_POLICY, INCLUSION_PROPERTY;
		long int Reads, Writes, Read_Miss, Write_Miss, Write_Backs, backInvalidation_WB;
		Block **block;
		void accessBlock(long int, char);
		Block *searchBlock(long int);
		Cache *L2_Cache;
		void L2Cache_Set(Cache *L2_Cache) {this->L2_Cache = L2_Cache;}
		int Hit;
		int Evicted;
		int Write_Back;
	
		Cache(long int Blocksize, long int Cache_Size, long int Assoc, string REPLACEMENT_POLICY, string INCLUSION_PROPERTY)
		{
			Reads = Writes = 0;
			Read_Miss = Write_Miss = Write_Backs  = 0;
			Replace_Num = 0;
			
			this->Assoc = Assoc;
			
			this->Blocksize = Blocksize;
			Offset_Size_i = log2(Blocksize);
			
			this->Cache_Size = Cache_Size;
			Offset_Size=(int)Offset_Size_i;

			this->INCLUSION_PROPERTY = INCLUSION_PROPERTY;
			this->Sets = (Cache_Size)/(Assoc * Blocksize);
			Index_Size_i = log2(Sets);
			Index_Size=(int)Index_Size_i;
			this->REPLACEMENT_POLICY = REPLACEMENT_POLICY;
			
			
			
			
			
			Tag_Size_i = Offset_Size - Index_Size;
			Tag_Size=64-Tag_Size_i;
			
			index_mask = mask(Index_Size + Offset_Size);
			tag_mask = mask(Tag_Size + Index_Size + Offset_Size);
			offset_mask = mask(Offset_Size);
			
			block=init(Sets,Assoc);
			L2_Cache = NULL;
		}
		
		long int mask(int N)
		{
			long int mask = 0;
			for(int i=0; i<N; i++)
			{
				mask <<=1;
				mask |=1;
			}
			return mask;
		}
		
};

Block *ReplaceBlock(long int,Cache *);
Block *InvalidBlock(long int,Cache *);

void Cache :: accessBlock(long int Address, char oprand)
{
	long int Index=((Address & index_mask)>>(Offset_Size));
	long int Tag=((Address & tag_mask)>>(Index_Size + Offset_Size));
	//count number of reads and writes
	if(oprand == 'w')
		Writes++;
	else
		Reads++;
	
	//initial initializations
	Way=-1;
	Hit=0;
	Evicted=0;
	Write_Back=0;
	
	//check if block present in cache
	Block *Cell = searchBlock(Address);
	
	Replace_Num++;
	
	if(Cell != NULL)					
	{
		//If address found in cache: HIT
		Hit=1;	

		if(REPLACEMENT_POLICY == "LRU")
		{
			Cell->Sequence_num=Replace_Num;
		}
		if(REPLACEMENT_POLICY == "FIFO")
		{
			if(Hit == 1)
			{
				Cell->Sequence_num=Cell->Sequence_num; 
			}
			else if (Hit ==0)
			{				
				Cell->Sequence_num=Replace_Num;
			} 
		}			
		
	}
	else							
	{
		//If address not found in cache: MISS		
		if(oprand == 'w')
		{
			Write_Miss++;
		}
		else
		{
			Read_Miss++;
		}
		Hit=0;
		
		if(nonexclusive_cache == true)
		{			
			Block *evictedBlock = ReplaceBlock(Index, this);
			
			if(evictedBlock->Flag== "Dirty")
			{		
				Write_Backs++;
				Write_Back=1;
			}
			evictedBlock->Flag="Valid";
			evictedBlock->Address=Address;
			evictedBlock->Tag=Tag;

			if(REPLACEMENT_POLICY == "LRU")
			{
				evictedBlock->Sequence_num=Replace_Num;
			}
			if(REPLACEMENT_POLICY == "FIFO")
			{
				if(Hit == 1)
				{
					evictedBlock->Sequence_num=evictedBlock->Sequence_num;
				}
				else if (Hit == 0)
				{
					evictedBlock->Sequence_num=Replace_Num;
				}
			}
			
			Cell= evictedBlock;
			
		}
	}
	if(oprand == 'w')
	{
		Cell->Flag="Dirty";
	}
}

Block* Cache::searchBlock(long int Address)
{
	long int Index=((Address & index_mask)>>(Offset_Size));
	long int Tagging=((Address & tag_mask)>>(Index_Size + Offset_Size));
	long int check;
	for(int i=0; i<(int)(Assoc); i++)
	{
		check=block[Index][i].Tag;
		if((block[Index][i].Flag != "Invalid"))
		{
			if(check== Tagging)
			{
				Way=i;
				return &(block[Index][i]);
			}		
		}
		
	}
	return NULL;
}

Block *ReplaceBlock(long int Index,Cache *obj)
{
	Block *block_toReplace = InvalidBlock(Index,obj);
	if(block_toReplace == NULL)
	{
		
		int w;
		long int min = obj->Replace_Num;
		if (obj->REPLACEMENT_POLICY == "LRU" )
		{
			for(int i=0; i<(int)(obj->Assoc); i++)
			{
				if(obj->block[Index][i].Sequence_num < min)	
				{
					min = obj->block[Index][i].Sequence_num;
					w = i;
				}
			}
		}
		if (obj->REPLACEMENT_POLICY == "FIFO")
		{
			for(int i=0; i<(int)(obj->Assoc); i++)
			{
				if(obj->block[Index][i].Sequence_num < min)	
				{
					min = obj->block[Index][i].Sequence_num;
					w = i;
				}
			}
		}
		
		obj->Way=w;
		long int Way = obj->Way;
		block_toReplace=&(obj->block[Index][Way]);
		
		
		obj->Evicted=1;
		obj->EvictedAddress=block_toReplace->Address;	
	}	
	return block_toReplace;
}

Block *InvalidBlock(long int Index,Cache *obj)
{
	for(int i=0; i<(int)(obj->Assoc); i++)
	{
		if(obj->block[Index][i].Flag == "Invalid")
		{
			obj->Way=i;
			return &(obj->block[Index][i]);
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	long int BLOCKSIZE, L1_SIZE, L1_ASSOC, L2_SIZE, L2_ASSOC, Address;
	char oprand;
	string REPLACEMENT_POLICY, INCLUSION_PROPERTY;
	char *trace_file = (char *)malloc(100);
	long int size_of_cache;
	Cache *L1, *L2;
	
	BLOCKSIZE = atol(argv[1]);
	L1_SIZE = atol(argv[2]);
	L1_ASSOC = atol(argv[3]);
	L2_SIZE = atol(argv[4]);
	L2_ASSOC = atol(argv[5]);

	int choice_replace = atoi(argv[6]);
	if (choice_replace==0)
		 REPLACEMENT_POLICY = "LRU";
	else if (choice_replace==1)
		 REPLACEMENT_POLICY = "FIFO";
	else if (choice_replace==2)
		 REPLACEMENT_POLICY ="optimal";

	int choice_inclusive = atoi(argv[7]);
	if (choice_inclusive==0)
		INCLUSION_PROPERTY = "non-inclusive";
	else if (choice_inclusive==1)
		INCLUSION_PROPERTY = "inclusive";
	else if (choice_inclusive==2)
		INCLUSION_PROPERTY = "exclusive";

	strcpy(trace_file, argv[8]);

	cout << "===== Simulator configuration =====" << endl;
	cout << "BLOCKSIZE:" << BLOCKSIZE << endl;
	cout << "L1_SIZE:" << L1_SIZE << endl;
	cout << "L1_ASSOC:" << L1_ASSOC << endl;
	cout << "L2_SIZE:" << L2_SIZE << endl;
	cout << "L2_ASSOC:" << L2_ASSOC << endl;
	cout << "REPLACEMENT POLICY:" <<  REPLACEMENT_POLICY << endl;
	cout << "INCLUSION PROPERTY:" << INCLUSION_PROPERTY << endl;
	cout << "trace_file:" << trace_file << endl;
	
	
	
	L1 = new Cache(BLOCKSIZE, L1_SIZE, L1_ASSOC, REPLACEMENT_POLICY, INCLUSION_PROPERTY);
	if(L2_SIZE!=0)
	{
		long int size_of_cache=BLOCKSIZE;
		L2 = new Cache(BLOCKSIZE, L2_SIZE, L2_ASSOC, REPLACEMENT_POLICY, INCLUSION_PROPERTY);
		L1->L2Cache_Set(L2);
	}

	long int set_of_cache_L1=0;
	long int set_of_cache_L2=0;

	ifstream fin;
	fin.open(trace_file);

	while(fin >> oprand >> hex >> Address)
	{
		L1->accessBlock(Address, oprand);
		long int size_of_cache=BLOCKSIZE;
		if(L2_SIZE != 0)
		{
			if (INCLUSION_PROPERTY == "non-inclusive")							
			{
				if(L1->Write_Back == 1)		
				{				
					long int size_of_cache=L1_SIZE;
					L2->accessBlock(L1->EvictedAddress, 'w');				
				}
				if(L1->Hit ==0)						
				{				
					L2->accessBlock(Address, 'r');	
				}					
			}		

			if(INCLUSION_PROPERTY == "inclusive")								
			{
				if(L1->Write_Back == 1)							
				{
					long int size_of_cache=L1_SIZE;
					L2->accessBlock(L1->EvictedAddress, 'w');				
					if(L2->Evicted == 1)	
					{
						//BackInvalidate if evicted from L2
						Block *Cell = L2->searchBlock(L2->EvictedAddress);
						if (Cell != NULL)
						{
							if(INCLUSION_PROPERTY == "inclusive" && L2->L2_Cache)
							{
								if(Cell->Flag == "Dirty")
								{
									L2->Write_Back=1;
									L2->backInvalidation_WB++;
								} 
							}
							Cell->Flag="Invalid";	
						}
					}
				}			
				if(L1->Hit ==0)							// If Access to L1 was a 'MISS' only, need to read 'Address' from L2 Cache
				{
					L2->accessBlock(Address, 'r');
					if(L2->Evicted == 1)
					{
						Block *Cell = L1->searchBlock(L1->EvictedAddress);
						if (Cell != NULL)
						{
							if(INCLUSION_PROPERTY == "inclusive" && L2->L2_Cache)
							{
								if(Cell->Flag == "Dirty")
								{
									L1->Write_Back=1;
									L1->backInvalidation_WB++;
								} 
							}
							Cell->Flag="Invalid";	
						}
					}						
				}	
			}

			if(INCLUSION_PROPERTY == "exclusive")								
			{
				if(L1->Hit == 1)
				{
					Block *Cell = L2->searchBlock(Address);
					if (Cell != NULL)
					{
						if(INCLUSION_PROPERTY == "inclusive" && L2->L2_Cache)
						{
							if(Cell->Flag == "Dirty")
							{
								L2->backInvalidation_WB++;
								L2->Write_Back=1;
							} 
						}
						Cell->Flag="Invalid";	
					}
				}
				{
					L2->accessBlock(L1->EvictedAddress, 'w');				
					if(L1->Write_Back == 0)						
						L2->searchBlock(L1->EvictedAddress)->Flag="Valid";	
				}
				if(L1->Hit == 0)							
				{
					nonexclusive_cache = false;					
					L2->accessBlock(Address, 'r');
					set_of_cache_L2=L2->Sets;
					nonexclusive_cache = true;
					if(L2->Hit == 1)						
					{
						if(L2->searchBlock(Address)->Flag== "Dirty")		
							L1->searchBlock(Address)->Flag="Dirty";		
						Block *Cell = L2->searchBlock(Address);
						if (Cell != NULL)
						{
							if(INCLUSION_PROPERTY == "inclusive" && L2->L2_Cache)
							{
								if(Cell->Flag == "Dirty")
								{
									L2->Write_Back=1;
									L2->backInvalidation_WB++;
								} 
							}
							Cell->Flag="Invalid";	
						}
					}
				}
				
			}
		}
	}

	fin.close();
	
	cout << "===== L1 contents =====" ;

	for(int i=0;i<10;i++)
	{
		cout << "\nSet     "<<i<<":      ";
		for(int j=0;j<L1_ASSOC; j++)
		{
			
			if(L1->block[i][j].Flag == "Dirty")
				cout<<hex<<L1->block[i][j].Tag<<dec<<" D  ";
			else
				cout<<hex<<L1->block[i][j].Tag<<dec<<"    ";
		}
		//cout<<endl;
	}
	for(int i=10;i<L1->Sets;i++)
	{

		cout << "\nSet     "<<i<<":     ";
		for(int j=0;j<L1_ASSOC; j++)
		{
			if(L1->block[i][j].Flag == "Dirty")
				cout<<hex<<L1->block[i][j].Tag<<dec<<" D  ";
			else
				cout<<hex<<L1->block[i][j].Tag<<dec<<"    ";
		}
		//cout<<endl;
	}
	if(L2 != NULL)

	{
		cout<<"\n===== L2 contents =====";
		for(int i=0; i<10;i++)
		{

			cout<<"\nSet     "<<i<<":     ";
			for(int j=0;j<L2_ASSOC; j++)
			{
				if(L2->block[i][j].Flag == "Dirty")
					cout<<hex<<L2->block[i][j].Tag<<dec<<" D  ";
				else
					cout<<hex<<L2->block[i][j].Tag<<dec<<"    ";
			}
			//cout<<endl;

		}
		for(int i=10; i<L2->Sets;i++)
		{

			cout<<"\nSet     "<<i<<":     ";
			for(int j=0;j<L2_ASSOC; j++)
			{
				if(L2->block[i][j].Flag == "Dirty")
					cout<<hex<<L2->block[i][j].Tag<<dec<<" D  ";
				else
					cout<<hex<<L2->block[i][j].Tag<<dec<<"    ";
			}
		}
	}

	
	cout << "\n===== Simulation Results (raw) =====" << endl;
	cout << "a. number of L1 reads:" << L1->Reads << endl;
	cout << "b. number of L1 read misses:" << L1->Read_Miss << endl;
	cout << "c. number of L1 writes:" << L1->Writes << endl;
	cout << "d. number of L1 write misses:" << L1->Write_Miss << endl;
	float L1_Miss_Read=(float)(((float)(L1->Read_Miss + L1->Write_Miss))/(L1->Reads + L1->Writes)) ;
	cout << "e. L1 miss rate:" << fixed << setprecision(6) << L1_Miss_Read<< endl;
	cout << "f. number of L1 writebacks:" << L1->Write_Backs << endl;
	long int Mem_Traffic=0;
	if(L2 != NULL)
	{			

		cout << "g. number of L2 reads:" << L2->Reads << endl;                   	
		cout << "h. number of L2 read misses:" << L2->Read_Miss << endl;                   
		cout << "i. number of L2 writes:" << L2->Writes << endl;
		cout << "j. number of L2 write misses:" << L2->Write_Miss << endl;
		float L2_Miss_Reads=(float)(((float)(L2->Read_Miss))/(L2->Reads));
		cout << "k. L2 miss rate:" << fixed << setprecision(6) << L2_Miss_Reads << endl;
		cout << "l. number of L2 writebacks:" << L2->Write_Backs << endl;
		
		if (choice_inclusive==1)
		{
			Mem_Traffic=L2->Read_Miss +  L2->Write_Miss+ L2->Write_Backs+ L1->backInvalidation_WB;
			cout << "m. total memory traffic:" << Mem_Traffic<< endl;
		}
		else 
		{
			Mem_Traffic= L2->Read_Miss +  L2->Write_Miss + L2->Write_Backs;
			cout << "m. total memory traffic:" <<Mem_Traffic<< endl;
		}
	}
	else
	{	
		
		cout << "g. number of L2 reads:" << 0 << endl;
		cout << "h. number of L2 read misses:" << 0 << endl;
		cout << "i. number of L2 writes:" << 0 << endl;
		cout << "j. number of L2 write misses:" << 0 << endl;
		cout << "k. L2 miss rate:" << 0 << endl;
		cout << "l. number of L2 writebacks:" << 0 << endl;
		Mem_Traffic = L1->Read_Miss +  L1->Write_Miss + L1->Write_Backs;
		cout << "m. total memory traffic:" << Mem_Traffic<< endl;
	}
	return 1;
}
