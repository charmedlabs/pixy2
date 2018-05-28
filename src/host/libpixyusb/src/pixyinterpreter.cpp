//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <string.h>
#include <stdio.h>
#include "pixyinterpreter.hpp"

#define  PIXY_ARRAY_SIZE                        21

PixyInterpreter::PixyInterpreter()
{
  receiver_ = NULL;
}

PixyInterpreter::~PixyInterpreter()
{
  close();
}

int PixyInterpreter::init()
{
  int USB_return_value;

  USB_return_value = link_.open();

  if(USB_return_value < 0) {
    return USB_return_value;
  }

  receiver_ = new ChirpReceiver(&link_, this);

  return 0;
}

void PixyInterpreter::close()
{
  if (receiver_)  
  {
    delete receiver_;
    receiver_ = NULL;
  }
}

int PixyInterpreter::get_blocks(int max_blocks, Block * blocks)
{
  uint16_t   number_of_blocks_to_copy;
  uint16_t   index;
  int        result;
  ChirpProc  exec_run;
  uint8_t    buffer[128];
  uint8_t *  data;
  uint8_t    data_type;
  uint32_t   response;
  uint32_t   data_length;


  // Check parameters //

  if(max_blocks < 0 || blocks == 0) {
    return PIXY_ERROR_INVALID_PARAMETER;
  }

  // Request block data from Pixy //

  buffer[0] = PIXY_SIG_ALL;
  buffer[1] = PIXY_ARRAY_SIZE;
  exec_run  = receiver_->getProc("ser_packet");
  result    = receiver_->callSync(exec_run,
                                  UINT8(PIXY_TYPE_REQUEST_BLOCKS),
                                  UINTS8(2, buffer),
                                  END_OUT_ARGS,
                                  & response,
                                  & data_type,
                                  & data_length,
                                  & data,
                                  END_IN_ARGS);

  // Validate response //

  if (result < 0)
  {
    printf ("libpixyusb: Error: callSync() returned %d\n", result);
    return 0;
  }

  if (data_type == PIXY_TYPE_RESPONSE_BLOCKS)
  {
    int  block_count;
    int  blocks_copied;

    // Validate block data length //

    if (data_length % sizeof (Block) != 0)
    {
      printf ("libpixyusb: Error: Unexpected block size (%d)\n", data_length);
      return 0;
    }

    // Transfer blocks to user buffer //

    block_count = data_length / sizeof (Block);

    // How many blocks should we copy? //
    if (block_count > max_blocks)
    {
      blocks_copied = max_blocks;
    }
    else
    {
      blocks_copied = block_count;
    }

    // Perform the copy operation //
    memcpy (blocks, data, sizeof (Block) * blocks_copied);

    return blocks_copied;
  }

  return 0;
}

int PixyInterpreter::get_vectors(/* TODO */)
{
  int        result;
  ChirpProc  exec_run;
  uint8_t    buffer[128];
  uint8_t *  data;
  uint8_t    data_type;
  uint32_t   response;
  uint32_t   data_length;

  exec_run  = receiver_->getProc("ser_packet");
  result = receiver_->callSync(exec_run,
                               UINT8(PIXY_TYPE_REQUEST_LINES),
                               UINTS8(0, buffer),
                               END_OUT_ARGS,
                               & response,
                               & data_type,
                               & data_length,
                               & data,
                               END_IN_ARGS);

  printf ("  result      = %d\n", result);
  printf ("  data_type   = %d\n", data_type);
  printf ("  response    = %d\n", response);
  printf ("  data_length = %d\n", data_length);;
  printf ("  data = ");

  {
     int i;

     for (i = 0; i != data_length; ++i)
     {
        printf("%02x ", data[i]);
     }
     printf ("\n");
  }
}

int PixyInterpreter::send_command(const char * name, ...)
{
  va_list arguments;
  int     return_value;

  va_start(arguments, name);
  return_value = send_command(name, arguments);
  va_end(arguments);

  return return_value;
}

int PixyInterpreter::send_command(const char * name, va_list args)
{
  ChirpProc procedure_id;
  int       return_value;
  va_list   arguments;

  va_copy(arguments, args);

  // Request chirp procedure id for 'name'. //
  procedure_id = receiver_->getProc(name);

  // Was there an error requesting procedure id? //
  if (procedure_id < 0) {
    // Request error //
    va_end(arguments);

    return PIXY_ERROR_INVALID_COMMAND;
  }

  // Execute chirp synchronous remote procedure call //
  return_value = receiver_->call(SYNC, procedure_id, arguments); 
  va_end(arguments);

  return return_value;
}

void PixyInterpreter::interpret_data(const void * chirp_data[])
{
  uint8_t  chirp_message;
  uint32_t chirp_type;
  if (chirp_data[0]) {

    chirp_message = Chirp::getType(chirp_data[0]);

    switch(chirp_message) {
      
      case CRP_TYPE_HINT:
        
        chirp_type = * static_cast<const uint32_t *>(chirp_data[0]);

        switch(chirp_type) {

          case FOURCC('B', 'A', '8', '1'):
            break;
          case FOURCC('C', 'C', 'Q', '1'):
            break;
          case FOURCC('C', 'C', 'B', '1'):
            interpret_CCB1(chirp_data + 1);
            break;
          case FOURCC('C', 'C', 'B', '2'):
            interpret_CCB2(chirp_data + 1);
            break;
          case FOURCC('C', 'M', 'V', '1'):
            break;
          default:
            printf("libpixy: Chirp hint [%u] not recognized.\n", chirp_type);
            break;
        }

        break;

      case CRP_HSTRING:

        break;
      
      default:
       
       fprintf(stderr, "libpixy: Unknown message received from Pixy: [%u]\n", chirp_message);
       break;
    }
  } 
}

void PixyInterpreter::interpret_CCB1(const void * CCB1_data[])
{
  uint32_t       number_of_blobs;
  const BlobA *  blobs;
  uint32_t       index;
  
  // Add blocks with normal signatures //
  
  number_of_blobs = * static_cast<const uint32_t *>(CCB1_data[3]);
  blobs           = static_cast<const BlobA *>(CCB1_data[4]);
  
  number_of_blobs /= sizeof(BlobA) / sizeof(uint16_t);
  
  add_normal_blocks(blobs, number_of_blobs);
}


void PixyInterpreter::interpret_CCB2(const void * CCB2_data[])
{
  uint32_t       number_of_blobs;
  const BlobA *  A_blobs;
  const BlobB2 *  B_blobs;
  uint32_t       index;

  // The blocks container will only contain the newest //
  // blocks                                            //
  blocks_.clear();

  // Add blocks with color code signatures //

  number_of_blobs = * static_cast<const uint32_t *>(CCB2_data[5]);
  B_blobs         = static_cast<const BlobB2 *>(CCB2_data[6]);
  
  number_of_blobs /= sizeof(BlobB2) / sizeof(uint16_t);
  add_color_code_blocks(B_blobs, number_of_blobs);

  // Add blocks with normal signatures //

  number_of_blobs = * static_cast<const uint32_t *>(CCB2_data[3]);
  A_blobs         = static_cast<const BlobA *>(CCB2_data[4]);
  
  number_of_blobs /= sizeof(BlobA) / sizeof(uint16_t);
  
  add_normal_blocks(A_blobs, number_of_blobs);
}

void PixyInterpreter::add_normal_blocks(const BlobA * blocks, uint32_t count)
{
  uint32_t index;
  Block    block;

  for (index = 0; index != count; ++index) {

    // Decode CCB1 'Normal' Signature Type //

    block.type      = PIXY_BLOCKTYPE_NORMAL;
    block.signature = blocks[index].m_model;
    block.width     = blocks[index].m_right - blocks[index].m_left;
    block.height    = blocks[index].m_bottom - blocks[index].m_top;
    block.x         = blocks[index].m_left + block.width / 2;
    block.y         = blocks[index].m_top + block.height / 2;

    // Angle is not a valid parameter for 'Normal'  //
    // signature types. Setting to zero by default. //
    block.angle     = 0;
      
    // Store new block in block buffer //

    if (blocks_.size() == PIXY_BLOCK_CAPACITY) {
      // Blocks buffer is full - replace oldest received block with newest block //
      blocks_.erase(blocks_.begin());
      blocks_.push_back(block);
    } else {
      // Add new block to blocks buffer //
      blocks_.push_back(block);
    }
  }
}

void PixyInterpreter::add_color_code_blocks(const BlobB2 * blocks, uint32_t count)
{
  uint32_t index;
  Block    block;
    
  for (index = 0; index != count; ++index) {

    // Decode 'Color Code' Signature Type //

    block.type      = PIXY_BLOCKTYPE_COLOR_CODE;
    block.signature = blocks[index].m_model;
    block.width     = blocks[index].m_right - blocks[index].m_left;
    block.height    = blocks[index].m_bottom - blocks[index].m_top;
    block.x         = blocks[index].m_left + block.width / 2;
    block.y         = blocks[index].m_top + block.height / 2;
    block.angle     = blocks[index].m_angle;

    // Store new block in block buffer //

    if (blocks_.size() == PIXY_BLOCK_CAPACITY) {
      // Blocks buffer is full - replace oldest received block with newest block //
      blocks_.erase(blocks_.begin());
      blocks_.push_back(block);
    } else {
      // Add new block to blocks buffer //
      blocks_.push_back(block);
    }
  }
}
